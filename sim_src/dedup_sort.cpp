/*
 * Sort the given scan log, save into output.
 * Sorted results can be processed much faster than raw log.
 */
#include <iostream>
#include <algorithm>
#include <fstream>
#include <string>
#include "dedup_types.hpp"
extern "C" {
#include "dedup_common.h"
}
using namespace std;

void usage(char *progname)
{
	pr_msg("This program sort a block log file by block hash.\n"
	       "if -r option is specified, then sort by offset."
	       "Usage: %s log_file output [-r]", progname);
}

bool block_compare(const Block &b1, const Block &b2)
{
	int res = memcmp(b1.cksum, b2.cksum, CKSUM_LEN);
	if (res >= 0)
		return false;
	return true;
}

bool compare_by_offset(const Block& left, const Block& right)
{
	return left.offset < right.offset;
}

int main(int argc, char **argv)
{
	if (argc < 3 || argc > 4) {
		usage(argv[0]);
		return 0;
	}

	Block blk;
	bool sort_by_offset = false;
	ifstream is;
	ofstream os;
	std::vector<Block> blocks;
	std::vector<Block>::iterator it;

	is.open(argv[1], std::ios_base::in | std::ios_base::binary);
	is.seekg(0, ios::end);
	blocks.reserve(is.tellg()/RECORD_SIZE);
	is.seekg(0, ios::beg);

	while (blk.load(is))
		blocks.push_back(blk);
	is.close();

	if (argc == 4)
		sort_by_offset = true;

	if (sort_by_offset)
		sort(blocks.begin(), blocks.end(), compare_by_offset);
	else
		sort(blocks.begin(), blocks.end());

	os.open(argv[2], std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
	for (it = blocks.begin(); it != blocks.end(); it++) {
		os.write((char *)(*it).cksum, CKSUM_LEN);
		os.write((char *)&(*it).file_id, sizeof((*it).file_id));
		os.write((char *)&((*it).size), sizeof((*it).size));
		os.write((char *)&((*it).offset), sizeof((*it).offset));
	}
	os.close();

	return 0;
}
