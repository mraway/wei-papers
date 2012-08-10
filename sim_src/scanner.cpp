#include "scanner.hpp"
#include "fs_reader.hpp"
#include <openssl/sha.h>

Scanner::Scanner(bool var, int kb, uint64_t boundary_size, std::ofstream *output, std::ofstream* dataout)
{
	is_var = var;
	size_in_kb = kb;
	avg_size = 0x400 * size_in_kb;	// size in KB
	Tmin = avg_size >> MIN_THRESHOLD;	// amount of bytes being skipped
	Tmax = avg_size << MAX_THRESHOLD;	// maxium block size
	D = avg_size - Tmin;	// primary divisor indicates the real average scan length
	Ddash = D >> 1;		// to avoid hard break, backup divisor is half of the primary divisor
	b1_offset = 0;
	new_start = 0;
	backup_break = 0;
    fix_boundary_size = boundary_size;
	ofs = output;
    ofsdata = dataout;
	if (is_var)
		s = rabin_init(POLY, RABIN_WINDOW_SIZE);
	else 
		s = NULL;
}

Scanner::~Scanner()
{
	ofs->flush();
    if(ofsdata != NULL)
        ofsdata->flush();
	if (is_var)
		rabin_free(s);
    cout << endl << "tot_blocks: " << tot_blocks << " tot_dup_blocks: " << tot_dup_blocks << endl;
}

void Scanner::addData(uint8_t *b1, uint8_t *b2, uint64_t size, bool eof)
{
	if (is_var)
		varScan(b1, b2, size, eof);
	else
		fixScan(b1, b2, size, eof);
}

void Scanner::fixScan(uint8_t *b1, uint8_t *b2, uint64_t size, bool eof)
{
	uint64_t start = 0, end = 0;
	while (start < size) {
		end = start + avg_size;
		bl.offset = start + b1_offset;	// the overall offset
		if (end > size) {	// at the end
			bl.size = size - start;
			SHA1(&b1[start], bl.size, (unsigned char *)bl.cksum);
		}
		else {
			bl.size = avg_size;
			SHA1(&b1[start], bl.size, (unsigned char *)bl.cksum);
		}
		addBlock();	// add result
		start = end;
	}
	b1_offset += size;		// update the overall offset
}

void Scanner::varScan(uint8_t *b1, uint8_t *b2, uint64_t size, bool eof)
{
	if (s == NULL)
		return;

	uint64_t p = 0;
	uint64_t rabinf;
	uint64_t offset;
	for (; p < size; p++) {
		offset = b1_offset + p;		// will always use absolute offset below

        // for fix boundary mode, this will enforce a hard break and reset rabin state
        if (fix_boundary_size != 0 && offset != 0 && (offset % fix_boundary_size == 0)) {
            if (new_start < offset)	// enforced block must contain some data
                addBreakpoint(b1, b2, offset - 1);
            new_start = offset;
            backup_break = 0;
            rabin_free(s);
            s = rabin_init(POLY, RABIN_WINDOW_SIZE);
        }

		rabinf = rabin_slide8(s, b1[p]);	// update rabin state
		if ((offset - new_start + 1) < Tmin)	// not at min size yet
			continue;
		if ((rabinf % Ddash) == Ddash - 1) {	// possible backup breakpoint
			backup_break = offset;
		}
		if ((rabinf % D) == D - 1) {	// found a breakpoint before Tmax
			addBreakpoint(b1, b2, offset);
			backup_break = 0;
			new_start = offset + 1;
			continue;
		}
		if ((offset - new_start + 1) < Tmax) {	// current byte didn't bring up a breakpoint
			continue;
		}
		if (backup_break != 0) {	// use backup break if we found one
			addBreakpoint(b1, b2, backup_break);
			new_start = backup_break + 1;
			backup_break = 0;
		}
		else {	// impose a hard break
			addBreakpoint(b1, b2, offset);
			new_start = offset + 1;
			backup_break = 0;
		}
	}

	if (eof && new_start < (size + b1_offset) )	// at the end of file, add remaining data as a block
		addBreakpoint(b1, b2, size + b1_offset - 1);

	b1_offset += size;	// add up to the overall progress
}

void Scanner::addBreakpoint(uint8_t *b1, uint8_t *b2, uint64_t offset)
{
	bl.offset = new_start;
	bl.size = offset - bl.offset + 1;

    if (bl.size == 0) {
        pr_msg("error: adding zero-sized block at offset %llu", bl.offset);
        return;
    }

    tot_blocks++;

	if (bl.offset >= b1_offset) {	// all data in buffer1
		SHA1(&b1[bl.offset - b1_offset], 
		     bl.size, (unsigned char *)bl.cksum);

            if(ofsdata != NULL){
                string tmp((char*)bl.cksum, CKSUM_LEN);
                map<string,string>::iterator it = datamap.find(tmp);
                if(it == datamap.end()){
                    datamap[tmp] = tmp;
                    ofsdata->write((char *)bl.cksum, CKSUM_LEN);
                    ofsdata->write((char *)&(bl.size), sizeof(bl.size));
                    ofsdata->write((char*)b1 + (bl.offset - b1_offset), bl.size);
                }else{
                    tot_dup_blocks++;
                }
            }
	}
	else {
		if (offset < b1_offset) {	// all in buffer2
			SHA1(&b2[bl.offset + READ_BUFFER_SIZE - b1_offset], 
			     bl.size, (unsigned char *)bl.cksum);

            if(ofsdata != NULL){
                string tmp((char*)bl.cksum, CKSUM_LEN);
                map<string,string>::iterator it = datamap.find(tmp);
                if(it == datamap.end()){
                    datamap[tmp] = tmp;
                    ofsdata->write((char *)bl.cksum, CKSUM_LEN);
                    ofsdata->write((char *)&(bl.size), sizeof(bl.size));
                    ofsdata->write((char*)b2 + (bl.offset + READ_BUFFER_SIZE - b1_offset), bl.size);
                }else{
                    tot_dup_blocks++;
                }
            }

		}
		else {	// half-half
			SHA_CTX ctx;
			SHA1_Init(&ctx);
			SHA1_Update(&ctx, &b2[bl.offset + READ_BUFFER_SIZE - b1_offset], 
				    b1_offset - bl.offset);
			SHA1_Update(&ctx, &b1[0], 
				    offset - b1_offset + 1);
			SHA1_Final(bl.cksum, &ctx);

            if(ofsdata != NULL){
                string tmp((char*)bl.cksum, CKSUM_LEN);
                map<string,string>::iterator it = datamap.find(tmp);
                if(it == datamap.end()){
                    datamap[tmp] = tmp;
                    ofsdata->write((char *)bl.cksum, CKSUM_LEN);
                    ofsdata->write((char *)&(bl.size), sizeof(bl.size));
                    ofsdata->write((char*)b2 + (bl.offset + READ_BUFFER_SIZE - b1_offset), b1_offset - bl.offset);
                    ofsdata->write((char*)b1,offset - b1_offset + 1);
                }else{
                    tot_dup_blocks++;
                }
            }
        }
	}
	addBlock();
}

void Scanner::addBlock()
{
	extern uint32_t file_id;
	ofs->write((char *)bl.cksum, CKSUM_LEN);
	ofs->write((char *)&file_id, sizeof(file_id));
	ofs->write((char *)&(bl.size), sizeof(bl.size));
	ofs->write((char *)&(bl.offset), sizeof(bl.offset));
}

