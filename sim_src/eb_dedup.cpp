/*
 * this program split list of blocks into large segments,
 * using archor to determine the boundary of segments
 */

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <sstream>

#include "dedup_types.h"

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
    while (seg.Load(is)) {
        binmap[seg.GetMinHashString()].AddSegment(seg);
    }

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
