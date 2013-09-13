/*
 * Read a snapshot recipe, dedup it against its parent snapshot and CDS
 * This simulation uses lru strategy at level 2
 */
#include <iostream>
#include <fstream>
#include <string>
#include "dedup_types.hpp"

using namespace std;

#define IN_PARENT 0x01		// 001
#define IN_CDS 0x02			// 010
#define IN_DIRTY_SEG 0x05	// 101

void usage(char *progname)
{
	pr_msg("This program read a snapshot recipe, dedup it against its parent snapshot and CDS.\n"
           "CDS option is mandatory but can accept an empty file, parent snapshot is optinal.\n"
           "This simulation uses LRU strategy at level 2\n"
	       "Usage: %s snapshot CDS [parent]", progname);
}

/*
 * Load a 2MB segment from trace
 */

bool load_segment(Segment& seg, ifstream& is)
{
    Block blk;
    seg.Init();
    while (blk.Load(is)) {
        if (blk.GetSize() == 0) {	// fix the zero-sized block bug in scanner
            pr_msg("ignore zero-sized block");
            continue;
        }
        seg.AddBlock(blk);
        if (seg.GetSize() >= 2*1024*1024)
            break;
    }
    seg.Final();
    if (seg.GetSize() == 0) 
        return false;
    else
        return true;
}

int main(int argc, char** argv)
{
    std::vector<Block> cds;
    std::vector<Block>::iterator it;
    Segment current_seg, parent_seg;
    ifstream current_input, parent_input, cds_input;
    ofstream output;
    bool has_parent = false;
    uint32_t i;
    Block blk;
    //uint64_t new_size = 0;

    if (argc < 3 || argc > 4) {
        usage(argv[0]);
        return 0;
    }

    current_input.open(argv[1], std::ios_base::in | std::ios_base::binary);
    if (!current_input.is_open()) {
        pr_msg("unable to open %s", argv[1]);
        exit(1);
    }
    cds_input.open(argv[2], std::ios_base::in | std::ios_base::binary);
    if (!cds_input.is_open()) {
        pr_msg("unable to open %s", argv[2]);
        exit(1);
    }

    if (argc == 4) {
        has_parent = true;
        parent_input.open(argv[3], std::ios_base::in | std::ios_base::binary);
        if (!parent_input.is_open()) {
            pr_msg("unable to open %s", argv[3]);
            exit(1);
        }
    }

    string outputname = argv[1];
    outputname += ".cdssim";
    output.open(outputname.c_str(), std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);

    // prepare CDS, assume it was sorted
    while (blk.Load(cds_input)) {
        cds.push_back(blk);
        //sort(cds.begin(), cds.end());
    }

    while(load_segment(current_seg, current_input)) {
        // prepare parent
        if (has_parent && load_segment(parent_seg, parent_input))
            parent_seg.SortByHash();

        // prepare current segment, 0 means non-dup
        for (i = 0; i < current_seg.blocklist_.size(); i ++)
            current_seg.blocklist_[i].file_id_ = 0;

        // level 1: dirty bit
        if (has_parent && current_seg == parent_seg) {
            for (i = 0; i < current_seg.blocklist_.size(); i ++)
                current_seg.blocklist_[i].file_id_ |= IN_PARENT;
        }
        // level 2: search dirty segment
        else if (has_parent) {
            for (i = 0; i < current_seg.blocklist_.size(); i ++)
                if (parent_seg.SearchBlock(current_seg.blocklist_[i]))
                    current_seg.blocklist_[i].file_id_ |= IN_DIRTY_SEG;
        }

        // level-3: search CDS
        for (i = 0; i < current_seg.blocklist_.size(); i ++) {
            if (std::binary_search(cds.begin(), cds.end(), current_seg.blocklist_[i]))
                current_seg.blocklist_[i].file_id_ |= IN_CDS;
        }

        // write to output
        for (i = 0; i < current_seg.blocklist_.size(); i++)
            current_seg.blocklist_[i].Save(output);
    }

    output.close();
    parent_input.close();
    cds_input.close();
    current_input.close();

    return 0;
}
