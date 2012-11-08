/*
 * Read sorted logs, analysis the dedup efficiency.
 */
#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <deque>
#include <sys/stat.h>
#include <sys/types.h>
#include "dedup_types.hpp"

double threshold = 0;

void usage(char *progname)
{
	pr_msg("This program read a list of trace files from stdin, generate CDS.\n"
	       "Usage: sorted_log_files | %s percentage cds_file", progname);
}

/*
 * this will sort records by count from high to low
 */
bool sort_by_count(const Block& left, const Block& right)
{
    return left.file_id_ > right.file_id_;
}

int main(int argc, char **argv)
{

	if (argc != 3) {
		usage(argv[0]);
		return 0;
	}

    threshold = atof(argv[1]);
    std::fstream cds_output;
    cds_output.open(argv[2], std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
    if (!cds_output.is_open()) {
        pr_msg("can't open cds file to write: %s", argv[2]);
        return -1;
    }

    // check temp directory
    struct stat myst;
    std::string cds_tmp_dir("./cdstmp/");
    std::fstream fs_array[256];

    if (stat(cds_tmp_dir.c_str(), &myst) == 0 && S_ISDIR(myst.st_mode)) {
        pr_msg("tmp dir exists, will use it instead of re-generate tmp files.");
        for (int i = 0; i < 256; ++ i) {
            // open existing merged results
            std::stringstream tmp_filename;
            tmp_filename << std::setfill('0');
            tmp_filename << cds_tmp_dir << "merge." << std::setw(3) << i;
            fs_array[i].open(tmp_filename.str().c_str(), std::fstream::in | std::fstream::binary);
            if (!fs_array[i].is_open()) {
                pr_msg("can't open %s", tmp_filename.str().c_str());
                return -1;
            }

            // extract cds
            fs_array[i].seekg(0, std::ios_base::end);
            size_t filesize = fs_array[i].tellg();
            fs_array[i].seekg(0, std::ios_base::beg);
            int num_to_copy = (filesize / RECORD_SIZE) * threshold;
            Block blk;
            for (int j = 0; j < num_to_copy; ++j) {
                blk.Load(fs_array[i]);
                blk.Save(cds_output);
            }

            fs_array[i].close();
        }
        cds_output.close();
        return 0;
    }

    // create tmp dir and files
    if (mkdir(cds_tmp_dir.c_str(), 0755) != 0) {
        pr_msg("cannot create tmp dir!");
        return -1;
    }
    for (int i = 0; i < 256; ++ i) {
        std::stringstream tmp_filename;
        tmp_filename << std::setfill('0');
        tmp_filename << cds_tmp_dir << std::setw(3) << i;
        fs_array[i].open(tmp_filename.str().c_str(), std::fstream::in | std::fstream::out | std::fstream::binary | std::fstream::trunc);
        if (!fs_array[i].is_open()) {
            pr_msg("can't open %s", tmp_filename.str().c_str());
            return -1;
        }
    }
    // split every input file into tmp files
    std::string filename;
    int num_files = 0;
    while (std::getline(std::cin, filename)) {
        if (filename == "end")
            break;
        num_files++;
        ifstream is;
        is.open(filename.c_str(), std::ios_base::in | std::ios_base::binary);
        if (!is.is_open()) {
            pr_msg("can't open %s", filename.c_str());
            return -1;
        }

        Block blk;
        while(blk.Load(is)) {
            if (blk.file_id_ != 0)
                continue; // only allow new data
            blk.Save(fs_array[blk.cksum_[0]]);
        }
    }


    for (int i = 0; i < 256; i++) {
        fs_array[i].clear();
        fs_array[i].seekg(0, std::ios_base::beg);
    }

    // process each tmp file
    for (int i = 0; i < 256; ++i)
    {
        Block blk;
        std::deque<Block> raw;
        std::deque<Block> merged;
        while (blk.Load(fs_array[i]))
            raw.push_back(blk);

        std::sort(raw.begin(), raw.end());

        for (int j = 0; j < raw.size(); j ++) {
            int count = 1;
            while ((j + 1) < raw.size() && raw[j] == raw[j + 1]) {
                ++ j;
                ++ count;
            }
            raw[j].file_id_ = count;
            merged.push_back(raw[j]);
        }

        std::sort(merged.begin(), merged.end(), sort_by_count);
        fs_array[i].close();

        // save merged results
        std::stringstream ss;
        ss << std::setfill('0');
        ss << cds_tmp_dir << "merge." << std::setw(3) << i;
        fs_array[i].open(ss.str().c_str(), std::fstream::out | std::ofstream::binary | std::ofstream::trunc);
        for (int j = 0; j < merged.size(); ++ j)
            merged[j].Save(fs_array[i]);
        fs_array[i].close();

        // save cds
        int num_cds_records = threshold * merged.size();
        for (int j = 0; j < num_cds_records; ++ j)
            merged[j].Save(cds_output);
    }

    cds_output.close();
	return 0;
}
