/*
 * Read sorted logs, analysis the dedup efficiency.
 */
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include "dedup_types.hpp"
extern "C" {
#include "dedup_common.h"
}
using namespace std;

#define FREQ_COUNT 10000
#define MERGE_BUFFER_SIZE 1440*1024

uint64_t total_size = 0, dedup_size = 0, total_blocks = 0, uni_blocks = 0;
Block last_blk;
static uint64_t dup_count = 1;
char (*bufs)[MERGE_BUFFER_SIZE];
int* buf_pos;
int* buf_size;
std::vector<ifstream*> iss;
std::map<uint64_t, uint64_t> blk_count;
std::map<uint64_t, uint64_t> size_count;
std::map<uint64_t, uint64_t> dedup_count;
unsigned long* buf_total;
Block* bls;
int* tree;
int num_files;
bool write_cds;
ofstream os;
uint64_t threshold = 2;

void usage(char *progname)
{
	pr_msg("This program performs complete deduplication over input files.\n"
           "If a threshold is given, will output blocks whose duplication count are equal or above threshold.\n"
	       "Usage: sorted_log_files | %s [threshold output]", progname);
}

int block_compare(const Block &b1, const Block &b2)
{
	if (b1.size == 0 || b2.size == 0)	// one stream has reached the end, so its size is zero
		return b2.size - b1.size;	// we consider the zero size block as the larger one
	else
		return memcmp(b1.cksum, b2.cksum, CKSUM_LEN);
}

char* hash2string(checksum_t cksum, char *buf, int size)
{
    for (int i = 0; i < CKSUM_LEN; i++) {
        sprintf(&buf[2 * i], "%02x", cksum[i]);
    }
    return buf;
}

void print_block(char* title, Block& blk)
{
    char buf[41];
    pr_msg("%s: file_id: %lu, size: %lu, offset: %llu, hash: %s", title, blk.file_id, blk.size, blk.offset, hash2string(blk.cksum, buf, 20));
}

int load_buffer(int which)
{
    iss[which]->read(bufs[which], MERGE_BUFFER_SIZE);
    buf_total[which] += iss[which]->gcount();
    return iss[which]->gcount();
}

void load_block(int which)
{
    // first check the buffer to see if need to load data,
    // if has no more data to load then return a zero block
    if (buf_pos[which] >= buf_size[which]) {
        if (buf_size[which] != MERGE_BUFFER_SIZE) {
            bls[which].size = 0;
            return;
        }
        else {
            buf_size[which] = load_buffer(which);
            if (buf_size[which] == 0) {
                bls[which].size = 0;
                return;
            }
            buf_pos[which] = 0;
        }
    }

    memcpy((char *)bls[which].cksum, &bufs[which][buf_pos[which]], CKSUM_LEN);
    buf_pos[which] += CKSUM_LEN;
    memcpy((char *)&bls[which].file_id, &bufs[which][buf_pos[which]], sizeof(bls[which].file_id));
    buf_pos[which] += sizeof(bls[which].file_id);
    memcpy((char *)&bls[which].size, &bufs[which][buf_pos[which]], sizeof(bls[which].size));
    buf_pos[which] += sizeof(bls[which].size);
    memcpy((char *)&bls[which].offset, &bufs[which][buf_pos[which]], sizeof(bls[which].offset));
    buf_pos[which] += sizeof(bls[which].offset);
}

void count(Block & bl)
{
    //print_block("count", bl);
    total_size += bl.size;
    total_blocks += 1;
    if (block_compare(bl, last_blk) == 0) {	// not a new block
	dup_count++;
    }
    else {	// encounter a new block, can finish counting the previous block
		dedup_size += bl.size;
		uni_blocks += 1;

		// write down the counter of last block
        blk_count[dup_count]++;
        size_count[dup_count] += last_blk.size * dup_count;
        dedup_count[dup_count] += last_blk.size;

        if (write_cds && dup_count >= threshold) {
            last_blk.offset = dup_count;
            last_blk.save(os);
        }

		// start counting a new block
		dup_count = 1;
	}
	last_blk = bl;
}

void adjust_tree(int new_item)
{
    int tmp, parent, small;
    parent = (new_item + num_files) / 2;
    small = new_item;
    while (parent > 0) {
        // compare with loser
        if (block_compare(bls[small], bls[tree[parent]]) >= 0) {
            tmp = small;
            small = tree[parent]; 
            tree[parent] = tmp;
        }
        parent = parent / 2;
    }
    tree[0] = small; 
}

int init_tree(int root)
{
    if (root == 0) {
        if (num_files == 1)		// need at least 2 files
            tree[0] = 0;
        else
            tree[0] = init_tree(1);
        return tree[0];
    }
     
    int left = 2 * root;
    int right = left + 1;
    if (left >= num_files)
        left = left - num_files;
    else
        left = init_tree(left);
    if (right >= num_files)
        right = right - num_files;
    else
        right = init_tree(right);

    if (block_compare(bls[left], bls[right]) > 0) {
        tree[root] = left;
        return right;
    }
    else {
        tree[root] = right;
        return left;
    }
}

int main(int argc, char **argv)
{
	last_blk.size = 0;
	total_size = 0;
	dedup_size = 0;
	total_blocks = 0;
	uni_blocks = 0;
    num_files = 0;
    write_cds = false;
    int i = 0;
    std::string filename;

	if (argc != 1 && argc != 3) {
		usage(argv[0]);
		return 0;
	}

    if (argc == 3) {
        write_cds = true;
        os.open(argv[2], std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
        threshold = atoi(argv[1]);
    }
        
    while (std::getline(std::cin, filename)) {
        num_files++;
        ifstream* is = new ifstream;
        is->open(filename.c_str(), std::ios_base::in | std::ios_base::binary);
        if (!is->is_open()) {
			pr_msg("can't open %s", filename.c_str());
			return 0;
		}
        iss.push_back(is);
    }

    bls = new Block[num_files];
    buf_pos = new int[num_files];
    buf_size = new int[num_files];
    bufs = new char[num_files][MERGE_BUFFER_SIZE];
    buf_total = new unsigned long[num_files];
    tree = new int[num_files];

    
	// load the first block and init the loser tree
	for (i = 0; i < num_files; i++) {
		buf_total[i] = 0;
		buf_size[i] = load_buffer(i);
		buf_pos[i] = 0;
		load_block(i);
		/*
		  do {
		  load_block(i);
		  } while (bls[i].file_id != 0 && bls[i].size != 0);	// ignore those dup with parents
		*/
	}

	init_tree(0);
    
	do {
        count(bls[tree[0]]);
        // at the end of merge, we send a zero-sized block to count()
        // so that the last block's counter can be written down
        if (bls[tree[0]].size == 0)
            break;
	load_block(i);
	/*
        do {
            load_block(tree[0]);
        } while (bls[tree[0]].file_id != 0 && bls[tree[0]].size != 0);	// ignore those dup with parents or cds
	*/
        adjust_tree(tree[0]);
	} while (true);
    
    // a zero-sized block is added at the beginning of merge, remove it
    blk_count[1]--;

    map<uint64_t, uint64_t>::iterator it;
    for (it = blk_count.begin(); it != blk_count.end(); it++) {
        if ((*it).second !=0)
            pr_msg("%llu %llu %llu %llu", (*it).first, (*it).second, size_count[(*it).first], dedup_count[(*it).first]);
    }

	for (i = 0; i < num_files; i++) {
		iss[i]->close();
        delete iss[i];
    }
    delete[] buf_pos;
    delete[] buf_size;
    delete[] buf_total;
    delete[] bufs;
	delete[] bls;
    delete[] tree;
	return 0;
}
