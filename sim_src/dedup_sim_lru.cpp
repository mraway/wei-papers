/*
 * Read a snapshot recipe, dedup it against its parent snapshot and CDS
 * This simulation uses lru strategy at level 2
 */
#include <iostream>
#include <fstream>
#include <string>
#include <exception>
#include "dedup_types.hpp"
#include "lru_cache.hpp"

using namespace std;

void usage(char *progname)
{
	pr_msg("This program read a snapshot recipe, dedup it against its parent snapshot and CDS.\n"
           "CDS option is mandatory but can accept an empty file, parent snapshot is optinal.\n"
           "This simulation uses LRU strategy at level 2\n"
	       "Usage: %s CDS cache_size snapshot [parent]", progname);
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
    if (seg.GetSize() == 0 || seg.blocklist_.size() == 0) 
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

    if (argc < 4 || argc > 5) {
        usage(argv[0]);
        return 0;
    }

    current_input.open(argv[3], std::ios_base::in | std::ios_base::binary);
    if (!current_input.is_open()) {
        pr_msg("unable to open %s", argv[1]);
        exit(1);
    }
    cds_input.open(argv[1], std::ios_base::in | std::ios_base::binary);
    if (!cds_input.is_open()) {
        pr_msg("unable to open %s", argv[2]);
        exit(1);
    }

    LruCache cache(atoi(argv[2]));

    if (argc == 5) {
        has_parent = true;
        parent_input.open(argv[4], std::ios_base::in | std::ios_base::binary);
        if (!parent_input.is_open()) {
            pr_msg("unable to open %s", argv[3]);
            exit(1);
        }
    }

    string outputname = argv[3];
    outputname += ".simlru";
    outputname += argv[2];
    output.open(outputname.c_str(), std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);

    // prepare CDS, assume it was sorted
    while (blk.Load(cds_input)) {
        cds.push_back(blk);
        //sort(cds.begin(), cds.end());
    }
    cds_input.close();

    while(load_segment(current_seg, current_input)) {
        // prepare parent
        if (has_parent && load_segment(parent_seg, parent_input))
            parent_seg.SortByHash();
        else {
            has_parent = false;	// parent may be shorter than current
        }

        // prepare current segment, 0 means non-dup
        for (i = 0; i < current_seg.blocklist_.size(); i ++)
            current_seg.blocklist_[i].file_id_ = 0;
        
        // level 1: dirty bit
        if (has_parent && current_seg == parent_seg) {
            for (i = 0; i < current_seg.blocklist_.size(); i ++) {
                current_seg.blocklist_[i].file_id_ |= IN_PARENT;
            }
        }
        // level 2: search dirty segment
        else if (has_parent) {
            cache.AddItems(parent_seg);
            for (i = 0; i < current_seg.blocklist_.size(); i ++)
                if (cache.SearchItem(current_seg.blocklist_[i])) {
                    current_seg.blocklist_[i].file_id_ |= IN_DIRTY_SEG;
                }
        }
        
        // level-3: search CDS
        for (i = 0; i < current_seg.blocklist_.size(); i ++) {
            if (!cds.empty() && 
                std::binary_search(cds.begin(), cds.end(), current_seg.blocklist_[i]))
                current_seg.blocklist_[i].file_id_ |= IN_CDS;
        }
        
        // write to output
        for (i = 0; i < current_seg.blocklist_.size(); i++)
            current_seg.blocklist_[i].Save(output);
    }

    pr_msg("dedup finish");
    output.close();
    parent_input.close();
    current_input.close();
    return 0;
}
