/*
 * Sort the given scan log, save into output.
 * Sorted results can be processed much faster than raw log.
 */
#include <iostream>
#include <algorithm>
#include <fstream>
#include <string>
#include <deque>
#include "dedup_types.hpp"

using namespace std;

void usage(char *progname)
{
	pr_msg("This program sort a block log file by block hash.\n"
	       "if -r option is specified, then sort by offset.\n"
	       "Usage: %s log_file output [-r]", progname);
}

bool compare_by_offset(const Block& left, const Block& right)
{
	return left.offset_ < right.offset_;
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
	std::deque<Block> blocks;
	std::deque<Block>::iterator it;

	is.open(argv[1], std::ios_base::in | std::ios_base::binary);

	while (blk.Load(is))
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
        it->Save(os);
	}
	os.close();

	return 0;
}
