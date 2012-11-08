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
	uint64_t new_data = 0, l1_data = 0, l2_data = 0, l3_data = 0;

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
            break;
        case 1:
            l1_data += bl.size_;
            break;
        case 2:
            l3_data += bl.size_;
            break;
        case 5:
            l2_data += bl.size_;
            break;
        default:
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
	pr_msg("%s: %llu blocks, %llu bytes, %.6f new, %.6f l1, %.6f l2, %.6f l3",
	       argv[1], nblocks, size, (float)new_data/size, (float)l1_data/size, 
           (float)l2_data/size, (float)l3_data/size);
	is.close();
	return 0;
}
