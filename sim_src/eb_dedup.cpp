/*
 * this program split list of blocks into large segments,
 * using archor to determine the boundary of segments
 */

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <sstream>

#include "trace_types.h"
#include "dedup_types.hpp"

using namespace std;

void usage(char *progname)
{
    pr_msg("This program group a list of segments into bins by their min-hash, then simulate the dedupe process in every bin.\n"
           "Usage: %s segment_file", progname);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        usage(argv[0]);
        return 0;
    }

    /*
     * step 1: read segment and group them into bins
     */
    ifstream is;
    is.open(argv[1], std::ios_base::in | std::ios_base::binary);
    if (!is.is_open()) {
        pr_msg("can't open %s", argv[1]);
        return 0;
    }

    map<string, Bin> binmap;
    map<string, Bin>::iterator it;
    Segment seg;
    uint64_t blocks = 0;
    uint64_t bytes = 0;
    while (seg.FromStream(is)) {
        binmap[seg.GetMinHash().ToString()].AddSegment(seg);
        blocks += seg.blocklist_.size();
        bytes += seg.size_;
    }
    cout << "Partition summary: blocks: " << blocks << " bytes: " << bytes << endl;

    /*
     * step 2: eliminate duplicates inside the bin
     */
    for (it=binmap.begin(); it != binmap.end(); it ++) {
        (*it).second.Deduplication();
        pr_msg("%s", (*it).second.ToString().c_str());
    }
    is.close();
    return 0;
}
