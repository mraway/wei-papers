#include <iostream>
#include <fstream>
#include <string>
#include "dedup_types.hpp"
extern "C" {
#include "dedup_common.h"
}
using namespace std;
#define RECORD_SIZE 36

void usage(char *s)
{
	pr_msg("%s logfile [-d]", s);
}

char* hash2string(checksum_t cksum, char *buf, int size)
{
	for (int i = 0; i < CKSUM_LEN; i++) {
		sprintf(&buf[2 * i], "%02x", cksum[i]);
	}
	return buf;
}

void print_block(const char* title, Block& blk)
{
	char buf[41];
	pr_msg("%s: file_id: %lu, size: %lu, offset: %llu, hash: %s", title, blk.file_id, blk.size, blk.offset, hash2string(blk.cksum, buf, 20));
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
	//, new_data = 0, parent_data = 0, cds_data = 0, both_data = 0;

	is.open(argv[1], std::ios_base::in | std::ios_base::binary);
	if (!is.is_open()) {
		pr_msg("open failed: %s", argv[1]);
		return 0;
	}

	while(bl.load(is)) {
		size += bl.size;
		nblocks++;
		/*
		  switch (bl.file_id) {
		  case 0:
		  new_data += bl.size;
		  break;
		  case 1:
		  parent_data += bl.size;
		  break;
		  case 2:
		  cds_data += bl.size;
		  break;
		  case 3:
		  both_data += bl.size;
		  break;
		  default:
		  //print_block("incorrect category", bl);
		  //pr_msg("file broken: %s", argv[1]);
		  break;
		  }
		*/
		if (show_detail)
			print_block("block", bl);

		if (bl.size == 0 || bl.size > 16384) {
			print_block("corrupted", bl);
			pr_msg("file broken: %s", argv[1]);
			/*
			  is.close();
			  return 0;
			*/
		}
	}
	pr_msg("%s: %llu blocks,  %llu bytes",
	       argv[1], nblocks, size);
	is.close();
	return 0;
}
