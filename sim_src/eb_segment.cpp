/*
 * this program split list of blocks into large segments,
 * we use the same method as TTTD to determine the boundary of segments
 */
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include "trace_types.h"
#include "dedup_types.hpp"

using namespace std;

void usage(char *progname)
{
    pr_msg("This program read a raw scan log, split the list of blocks into variable-sized segments.\n"
           "The output of segments are partitioned by their min-hash.\n"
           "Usage: %s log_file output_dir", progname);
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        usage(argv[0]);
        return 0;
    }

    //int seg_size = atoi(argv[1]);
    //int tmin = seg_size / 4;
    //int tmax = seg_size * 4;
    //uint32_t divisor = seg_size - tmin;
    ifstream is;
    ofstream os[256];
    Block blk;
    Segment seg;
    //int nblocks = 0;

    is.open(argv[1], std::ios_base::in | std::ios_base::binary);
    for (int i = 0; i < 256; i++) {
        stringstream output_file;
        output_file << argv[2] << "/" << i;
        os[i].open(output_file.str().c_str(), std::ios_base::out | std::ios_base::binary | std::ios_base::app);
    }

    //seg.Init();
    while (seg.LoadFixSize(is)) {//(blk.Load(is)) {
        //seg.AddBlock(blk);
        //nblocks ++;

        //if (nblocks < tmin)
        //    continue;

        //uint32_t first4;
        //memcpy(&first4, blk.cksum_, 4);
        //if (first4 % divisor == 0) {
        //    seg.Final();	// break here
        //    seg.Save(os[seg.GetMinHash()[CKSUM_LEN - 1]]);
        //    seg.Init();
        //    nblocks = 0;
        //    continue;
        //}

        //if (nblocks < tmax)
        //    continue;

        //seg.Final();	// too big, enforce a break
        seg.SaveRaw(os[seg.GetMinHash().Middle4Bytes() % 256]);
        //seg.Init();
        //nblocks = 0;
    }

    //if (nblocks != 0) {	// the tail
    //    seg.Final();
    //    seg.Save(os[seg.GetMinHash()[CKSUM_LEN - 1]]);
    //}

    for (int i = 0; i < 256; i ++)
        os[i].close();
    is.close();
    return 0;
}

