/*
 * Read a snapshot recipe, dedup it against its parent snapshot and CDS
 * This simulation uses lru cache at level 3
 */
#include <iostream>
#include <fstream>
#include <string>
#include "dedup_types.hpp"
#include "lru_cache.hpp"

using namespace std;

void usage(char *progname)
{
	pr_msg("This program read a snapshot recipe, dedup it against its parent snapshot and CDS.\n"
           "CDS option is mandatory but can accept an empty file, parent snapshot is optinal.\n"
           "This simulation uses LRU cache at level 3\n"
	       "Usage: %s snapshot cds_file cds_cache_size [parent]", progname);
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
    if (argc < 4 || argc > 5) {
        usage(argv[0]);
        return 0;
    }

    std::vector<Block> cds;
    std::vector<Block>::iterator it;
    Segment current_seg, parent_seg;
    ifstream current_input, parent_input, cds_input;
    ofstream output;
    bool has_parent = false;
    uint32_t i;
    Block blk;

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

    if (argc == 5) {
        has_parent = true;
        parent_input.open(argv[4], std::ios_base::in | std::ios_base::binary);
        if (!parent_input.is_open()) {
            pr_msg("unable to open %s", argv[3]);
            exit(1);
        }
    }

    CdsLruCache cds_cache(atoi(argv[3]));
    std::string cache_file = "./old_cache_";
    cache_file += argv[3];
    std::ifstream old_cache(cache_file.c_str(), std::ios_base::in | std::ios_base::binary);
    if (old_cache.is_open())
        cds_cache.Load(old_cache);
    old_cache.close();

    string outputname = argv[1];
    outputname += ".cdslru";
    outputname += argv[3];
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
        for (int i = 0; i < current_seg.blocklist_.size(); i ++) {
            if (cds_cache.SearchItem(current_seg.blocklist_[i])) {
                current_seg.blocklist_[i].file_id_ |= IN_CDS_CACHE;
            }
            else if (std::binary_search(cds.begin(), cds.end(), current_seg.blocklist_[i])) {
                current_seg.blocklist_[i].file_id_ |= IN_CDS;
                cds_cache.AddItem(current_seg.blocklist_[i]);
            }
        }

        // write to output
        for (i = 0; i < current_seg.blocklist_.size(); i++)
            current_seg.blocklist_[i].Save(output);
    }
    
    ofstream new_cache(cache_file.c_str(), std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
    if (new_cache.is_open())
        cds_cache.Save(new_cache);
    new_cache.close();
    output.close();
    parent_input.close();
    cds_input.close();
    current_input.close();

    return 0;
}










