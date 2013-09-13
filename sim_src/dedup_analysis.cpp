#include <iostream>
#include <fstream>
#include <string>
#include "dedup_types.hpp"

using namespace std;
#define RECORD_SIZE 36

void usage(char *s)
{
	pr_msg("%s logfile [-d]", s);
}

char* hash2string(Checksum cksum, char *buf, int size)
{
	for (int i = 0; i < CKSUM_LEN; i++) {
		sprintf(&buf[2 * i], "%02x", cksum[i]);
	}
	return buf;
}

void print_block(const char* title, Block& blk)
{
	char buf[41];
	pr_msg("%s: file_id: %lu, size: %lu, offset: %llu, hash: %s", title, blk.file_id_, blk.size_, blk.offset_, hash2string(blk.cksum_, buf, 20));
}

int main(int argc, char **argv)
{
	if (argc < 2 || argc > 3) {
		usage(argv[0]);
		return 1;
	}

	bool show_detail = false;

	if (argc == 3) {
		show_detail = true;
	}

	ifstream is;
	Block bl;
	uint64_t size = 0, nblocks = 0;
	uint64_t new_data = 0, l1_data = 0, l2_data = 0, l3_data = 0, l3_cache = 0, other_data = 0;
    uint64_t new_blocks = 0, l1_blocks = 0, l2_blocks = 0, l3_blocks = 0, l3_cache_blocks = 0, other_blocks = 0;
    uint64_t gb = 1024 * 1024 * 1024;

	is.open(argv[1], std::ios_base::in | std::ios_base::binary);
	if (!is.is_open()) {
		pr_msg("open failed: %s", argv[1]);
		return 0;
	}

	while(bl.Load(is)) {
		size += bl.size_;
		nblocks++;
		
        switch (bl.file_id_) {
        case 0:
            new_data += bl.size_;
            new_blocks += 1;
            break;
        case IN_PARENT:
            l1_data += bl.size_;
            l1_blocks += 1;
            break;
        case IN_CDS:
            l3_data += bl.size_;
            l3_blocks += 1;
            break;
        case IN_DIRTY_SEG:
            l2_data += bl.size_;
            l2_blocks += 1;
            break;
        case (IN_PARENT | IN_CDS):
            l1_data += bl.size_;
            l1_blocks += 1;
            break;
        case (IN_DIRTY_SEG | IN_CDS):
            l2_data += bl.size_;
            l2_blocks += 1;
            break;            
        // case IN_CDS_CACHE:
        //     l3_data += bl.size_;
        //     l3_cache += bl.size_;
        //     break;
        default:
            other_data += bl.size_;
            other_blocks += 1;
            //print_block("incorrect category", bl);
            //pr_msg("file broken: %s", argv[1]);
            break;
        }
		
		if (show_detail)
			print_block("block", bl);

		if (bl.size_ == 0 || bl.size_ > 16384) {
			print_block("corrupted", bl);
			pr_msg("file broken: %s", argv[1]);
			/*
			  is.close();
			  return 0;
			*/
		}
	}
	pr_msg("%s:", argv[1]);
    pr_msg("Total: %llu blocks, %llu bytes", nblocks, size);
    pr_msg("L1: %llu blocks, %llu bytes", l1_blocks, l1_data);
    pr_msg("L2: %llu blocks, %llu bytes", l2_blocks, l2_data);
    pr_msg("L3: %llu blocks, %llu bytes", l3_blocks, l3_data);
    pr_msg("New: %llu blocks, %llu bytes", new_blocks, new_data);
    pr_msg("Other: %llu blocks, %llu bytes", other_blocks, other_data);
    pr_msg("Add: %llu blocks, %llu bytes", 
           l1_blocks + l2_blocks + l3_blocks + new_blocks + other_blocks, 
           l1_data + l2_data + l3_data + new_data + other_data);
	is.close();
	return 0;
}
