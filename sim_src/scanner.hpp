/*
 * Split data stream into variable size blocks
 * using content defined breakpoint algorithm.
 * For simplification, variable and fix scan
 * all use the same result format.
 */

#ifndef _SCANNER_H_
#define _SCANNER_H_

extern "C" {
#include "rabin.h"
}
#include "dedup_types.hpp"

#define MIN_THRESHOLD 1	// min threshold is half of the average size
#define MAX_THRESHOLD 2	// max threshold is 4 times of average size

using namespace std;

#include <map>

class Scanner
{
public:
	bool is_var;		// variable size of fixed
	uint32_t size_in_kb;	// the given average size in KB

	Scanner(bool var, int kb, uint64_t boundary_size, std::ofstream *output, std::ofstream *dataout);
	~Scanner();

	/*
	 * this is the main scanning procedure,
	 * buffer1 contains the new data of given size,
	 * buffer2 is the old data which carry over
	 * the remain data of last scan.
	 */
	void addData(uint8_t *b1, uint8_t *b2, uint64_t size, bool eof);

private:
	uint32_t avg_size;		// average block size in bytes
	uint32_t Tmin, Tmax;	// min & max threshold
	uint32_t D, Ddash;		// primary & backup divisor
	uint64_t new_start;		// position of the last breakpoint
	uint64_t backup_break;	// position of the last backup breakpoint
	uint64_t b1_offset;		// buffer1's position in file
	struct rabin_state *s;	// for rabin fingerprinting
	Block bl;				// the latest scan result
    uint64_t fix_boundary_size;	// for fix boundary mode
	std::ofstream *ofs;
	std::ofstream *ofsdata;
    std::map<std::string, std::string> datamap;
    uint32_t tot_blocks;
    uint32_t tot_dup_blocks;

	/*
	 * split data into fix size blocks,
	 * we assume the size of data is integer times of
	 * block size (2/4/8 KB), unless we reach the end of file.
	 */
	void fixScan(uint8_t *b1, uint8_t *b2, uint64_t size, bool eof);

	/*
	 * TTTD algorithm
	 */
	void varScan(uint8_t *b1, uint8_t *b2, uint64_t size, bool eof);

	/*
	 * create a block when a breakpoint is found
	 */
	void addBreakpoint(uint8_t *b1, uint8_t *b2, uint64_t offset);

	/*
	 * add a new block to the list
	 */
	void addBlock();
};
#endif
