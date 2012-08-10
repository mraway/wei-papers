#include "fs_reader.hpp"

using namespace std;
#ifdef USE_PANGU
using namespace apsara;
using namespace apsara::pangu;
using namespace apsara::security;
#endif
/*************************FSReader*************************/
std::string FSReader::getNextEntry()
{
	if (is_dir && index < entities.size())
		return entities[index++];
	else
		return "";
}

bool FSReader::open(const std::string& path) 
{
	return false;
}

uint64_t FSReader::readData(uint8_t *buffer, uint64_t length, uint64_t offset) 
{
	return 0;
}

void FSReader::close()
{
	return;
}

/***********************LocalFSReader**********************/
/*
 * Note: using native sys calls bring portbility issues.
 * Todo: switch to boost library.
 */
bool LocalFSReader::open(const std::string& path)
{
	name = path;

	struct stat st;
	if (lstat(name.c_str(), &st) != 0) {	// try to access
		pr_msg("lstat %s: %m", name.c_str());
		return false;
	}

	if (S_ISDIR(st.st_mode)) {	// open directory and read all its entries
		is_dir = true;

		DIR* dir = opendir(name.c_str());	// open
		if (dir == NULL) {
			pr_msg("opendir %s: %m", name.c_str());
			return false;
		}

		struct dirent *dirent;
		while ((dirent = readdir(dir)) != NULL) {	// read
			if ((strcmp(dirent->d_name, ".") == 0 || strcmp(dirent->d_name, "..") == 0))
				continue;	// skip . & ..
			entities.push_back(std::string(dirent->d_name));
		}

		closedir(dir);	// close
		return true;
	}


	if (S_ISREG(st.st_mode)) {	// get file's metadata (size), open it for read
		is_dir = false;
		size = st.st_size;
		is.open(name.c_str(), ios::binary);
		return true;
	}

	return false;
}

uint64_t LocalFSReader::readData(uint8_t *buffer, uint64_t length, uint64_t offset)
{
	if (is_dir || is.fail())	// either eof or fail after last read
		return 0;
	is.read((char *)buffer, length);
	return is.gcount();
}

void LocalFSReader::close()
{
	if (!is_dir && is.is_open())
		is.close();
}

LocalFSReader::~LocalFSReader()
{
	close();
}

/*************************PanguReader***********************/
#ifdef USE_PANGU
bool PanguReader::open(const std::string& path)
{
	name = path;
	if (*(name.rbegin()) == '/') {	// read directory
		is_dir = true;
		try {
			//bool have_more = true;	// 0x7FFFFFFF shoule be big enough
			std::string last_read = "";	// start from beginning
			FileSystem::GetInstance()->ListDirectory(name, last_read, 0x7FFFFFFF, entities);
		}
		catch (ExceptionBase& e) {
			cerr << "Open dir:" << e.what() << "\n";
			return false;
		}
		catch (...) {
			return false;
		}
	}
	else {	// open file. get its size. TODO: log file may cause problems
		is_dir = false;
		apsara::pangu::FileMeta meta;
		try {
			FileSystem::GetInstance()->GetFileMeta(name, meta);
			size = meta.fileLength;
			file_type = FILE_TYPE_FLAG_MASK(meta.fileFlag);
			switch (file_type) {
			case NORMAL_FILE_TYPE_FLAG:
				normal_is = FileSystem::GetInstance()->Open4Read(name);
				break;
#ifdef PANGU_RANDOM_ACCESS
			case RANDOM_ACCESS_FILE_TYPE_FLAG:
				random_is = FileSystem::GetInstance()->Open4RandomReadOnly(name);
				break;
#endif	/* PANGU_RANDOM_ACCESS */
			case LOG_FILE_TYPE_FLAG:
				log_is = FileSystem::GetInstance()->OpenLog4Read(name);
				break;
			default:
				pr_msg("Unknown file type: %s", name.c_str());
				return false;
			}
		}
		catch (ExceptionBase& e) {
			cerr << "Open file:" << e.what() << "\n";
			return false;
		}
		catch (...) {
			return false;
		}
	}
	return true;
}

uint64_t PanguReader::readData(uint8_t *buffer, uint64_t length, uint64_t offset)
{
	if (is_dir)
		return 0;
	try {
		switch (file_type) {
		case NORMAL_FILE_TYPE_FLAG:
			return normal_is->Read(buffer, length);
#ifdef PANGU_RANDOM_ACCESS
		case RANDOM_ACCESS_FILE_TYPE_FLAG:
			return random_is->Read(buffer, length, offset);
#endif	/* PANGU_RANDOM_ACCESS */
		case LOG_FILE_TYPE_FLAG:
			return log_is->ReadLog(buffer, length);
		default:
			pr_msg("Unknown file type: %s", name.c_str());
			return 0;
		}
	}
	catch (ExceptionBase& e) {
		cerr << "Read data:" << e.what() << '\n';
	}
	catch (...) {
		return 0;
	}

	return 0;
}

void PanguReader::close()
{
	if(is_dir)
		return;
	switch (file_type) {
	case NORMAL_FILE_TYPE_FLAG:
		normal_is->Close();
		return;
#ifdef PANGU_RANDOM_ACCESS
	case RANDOM_ACCESS_FILE_TYPE_FLAG:
		random_is->Close();
		return;
#endif	/* PANGU_RANDOM_ACCESS */
	case LOG_FILE_TYPE_FLAG:
		log_is->Close();
		return;
	default:
		pr_msg("Unknown data type: %s", name.c_str());
		return;
	}
}

PanguReader::~PanguReader()
{
}
#endif
