/*
 * classes for reading local or cloud file systems
 */
#ifndef _FS_READER_H_
#define _FS_READER_H_

#include "dedup_types.hpp"
#include <string>

#ifdef USE_PANGU
#include "apsara/common/base.h"	// for pangu
#include "apsara/common/timer.h"
#include "apsara/pangu.h"
#endif

#include <iostream>	// for local file access
#include <fstream>
#include <dirent.h>
#include <sys/stat.h>

using namespace std;
#ifdef USE_PANGU
using namespace apsara;
using namespace apsara::pangu;
#endif

/*
 * Base class for the interface of reading a file system entity
 */
class FSReader
{
public:
	bool is_dir;		// dir or file?
	uint64_t size;		// file size
	std::string name;	// fill path and name of the target
	checksum_t cksum;

	FSReader() { size = 0; index = 0; is_dir = false; };
	virtual ~FSReader() {};

	/*
	 * Given the name of a file system entity, read its metadata.
	 * If it's a directory, set the is_dir flag, read all its entries,
	 * if it's a file, open it, read its size.
	 * Return false if unable to access its metadata.
	 */
	virtual bool open(const std::string& path);

	/*
	 * Return the next entity in current directory
	 */
	virtual std::string getNextEntry();

	/*
	 * Read the specified amount of bytes from file.
	 * Return the actual number of bytes.
	 */
	virtual uint64_t readData(uint8_t *buffer, uint64_t length, uint64_t offset);

	/*
	 * Close the file
	 */
	virtual void close();

protected:
	std::vector<std::string> entities;	// list of dir entities
	unsigned int index;	// index used to read dir entities
};

class LocalFSReader : public FSReader
{
public:
	LocalFSReader() {};
	virtual ~LocalFSReader();
	virtual bool open(const std::string& path);
	virtual uint64_t readData(uint8_t *buffer, uint64_t length, uint64_t offset);
	virtual void close();
private:
	ifstream is;
};

#ifdef USE_PANGU
class PanguReader : public FSReader
{
public:
	PanguReader() {};
	virtual ~PanguReader();
	virtual bool open(const std::string& path);
	virtual uint64_t readData(uint8_t *buffer, uint64_t length, uint64_t offset);
	virtual void close();
private:
	apsara::pangu::FileTypeFlag file_type;		// pangu file type
	apsara::pangu::FileInputStreamPtr normal_is;
	apsara::pangu::LogFileInputStreamPtr log_is;
#ifdef PANGU_RANDOM_ACCESS
	apsara::pangu::RandomAccessFilePtr random_is;	// only for new pangu
#endif	/* PANGU_RANDOM_ACCESS */
};
#endif	/* USE_PANGU */

#endif	/* _FS_READER_H_ */
