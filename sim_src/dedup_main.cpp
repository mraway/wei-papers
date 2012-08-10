/*
 * This program scans a given directory (local or in cloud)
 * using the specified method (fix or variable chunk size).
 * Results are stored into a local sqlite database.
 */

#include "dedup_types.hpp"
#include "scanner.hpp"
#include "fs_reader.hpp"
#include <unistd.h>	// getopt
#include <stdlib.h>
#include <openssl/sha.h>
#include <map>


using namespace std;
#ifdef USE_PANGU
using namespace apsara;
using namespace apsara::pangu;
using namespace apsara::security;

DEFINE_FLAG_STRING(CapabilityFile, "File name whose contents will be used as capability", "");
#endif	/* USE_PANGU */

bool scan_all;					// scan once for every mode: variable & fix, 4/8/16 KB
bool is_var;					// variable of fix size chunk
int block_size;					// block size in KB
file_system_t ft;				// file system type
uint32_t file_id;				// file id used to map block to file
std::map<std::string, ofstream *> stream_map;	// map scan mode to output stream
std::map<std::string, ofstream *> stream_mapdata;	// map scan mode to output stream
std::string path;				// path to scan with
std::string prefix;				// only scan files with this prefix
std::string logname;			// log file name
uint64_t total_size;			// count total scanned bytes
uint64_t total_files;			// count total scanned files
std::string dataname;
bool use_fix_boundary;			// fix boundary mode
uint64_t fix_boundary_size;		// size between fix boundaries
bool meta_only;					// if true, will only traverse metadata
uint64_t min_size;				// file smaller than this size will be skipped
int scan_skip;					// skip this number of files at beginning
uint64_t scan_count;			// quit after scanning these many files, 0 means endless

/*
 * we use two buffers for file reading because
 * variable size chunking algorithm need to keep
 * some data at the end of previous buffer
 */
uint8_t buffer1[READ_BUFFER_SIZE];
uint8_t buffer2[READ_BUFFER_SIZE];
uint8_t *p1 = buffer1;
uint8_t *p2 = buffer2;

/*
 * print usage
 */
void usage(char *s)
{
	pr_msg("Usage: %s [OPTIONS] -l|p path -o log_file\n"
	       "Scan files in the specified path, save results to the log file.\n"
	       "\nOptions:\n"
	       "\t-l path\t\t Read from local file system.\n"
	       "\t-p path\t\t Read from pangu file system.\n"
           "\t-a path\t\t Read from alimail's attachment storage.\n"
           "\t-x prefix of filename.\n"
	       "\t-o log_file\t Output to log file.\n"
	       "\t-d data_file\t Data output to data file.\n"
	       "\t-f size(KB)\t Using fix size block.\n"
	       "\t-v size(KB)\t Using variable size block.\n"
	       "\t-m\t\t Only look meta-data.\n"
	       "\t-s size\t\t Ignore files smaller than given size.\n"
	       "\t-k number\t Skip the given number of files at beginning.\n"
	       "\t-n number\t Stop after having scanned given number of files.\n"
	       "\t-t\t\t Format numbers in thousands' grouping.\n"
	       "\t-h\t\t Help message.\n"
	       , s);
}

/*
 * Read data from an opened file system object,
 * we let p1 always point to the latest data, p2
 * always point to the old data. However, scanner 
 * may still need old buffer until new buffer is scanned.
 */
uint64_t load_data(FSReader *fsr, uint8_t **pp1, uint8_t **pp2, SHA_CTX *ctx, uint64_t offset) {
	uint8_t* temp = *pp2;
	*pp2 = *pp1;
	*pp1 = temp;
	uint64_t nbytes = fsr->readData(*pp1, READ_BUFFER_SIZE, offset);
	SHA1_Update(ctx, *pp1, nbytes);
	return nbytes;
}

/*
 * Given scanner's parameter: variable or fixed, and block size,
 * return the corresponding string of scan mode
 */
char* get_mode(char *s, bool var, int size)
{
	s[0] = var ? 'v' : 'f';
	sprintf(&s[1], "%d", size);
	return s;
}

/*
 * Return the full logfile name by scan mode: logname.scanmode
 */
std::string get_logfile_name(bool is_var, int size, bool fix_boundary)
{
	char mode[8];
    if (fix_boundary)
        return logname + ".b" + get_mode(mode, is_var, size);
    else
        return logname + "." + get_mode(mode, is_var, size);
}

std::string get_datafile_name(bool is_var, int size, bool fix_boundary)
{
	char mode[8];
    if (fix_boundary)
        return dataname + ".data.b" + get_mode(mode, is_var, size);
    else
        return dataname + ".data." + get_mode(mode, is_var, size);
}


/*
 * Append the file information into logname.file
 */
void save_file(FSReader *fsr)
{
	total_files++;
	total_size += fsr->size;
	ofstream *os = stream_map["file"];
	if (!os->is_open()) {
		pr_msg("file not opened");
		return;
	}
	file_id++;
	size_t len = fsr->name.size();
	os->write((char *)&file_id, sizeof(file_id));
	os->write((char *)&fsr->size, sizeof(fsr->size));
	os->write((char *)fsr->cksum, CKSUM_LEN);
	os->write((char *)&len, sizeof(size_t));
	os->write(fsr->name.c_str(), fsr->name.size());
	os->flush();
}

/*
 * Append scan results to logname.scanmode
 */
/*
void save_results(Scanner *s)
{
	ofstream *os = stream_map[get_logfile_name(s->is_var, s->size_in_kb)];
	if (!os->is_open()) {
		pr_msg("file not opened");
		return;
	}
	std::vector<Block>::iterator it;
	for (it = s->results.begin(); it != s->results.end(); it++) {
		os->write((char *)(*it).cksum, CKSUM_LEN);
		os->write((char *)&file_id, sizeof(file_id));
		os->write((char *)&((*it).size), sizeof((*it).size));
		os->write((char *)&((*it).offset), sizeof((*it).offset));
	}
	os->flush();
}
*/

/*
 * scan the given directory recursively, add file system entries to database
 */
void scan(std::string path) {
	pr_msg("%s", path.c_str());
	FSReader *fr;
	switch (ft) {
	case LOCAL:
		fr = new LocalFSReader;
		break;
#ifdef USE_PANGU
	case PANGU:
		fr = new PanguReader;
		break;
#endif	/* USE_PANGU */
	default:
		return;
	}

	int nretry = MAX_RETRY;	// retry a few times if fail to open
	int waitsec = 1;
	while (!fr->open(path)) {
		if (nretry-- <= 0)
			return;
		pr_msg("will retry %d times in %d seconds...", nretry, waitsec);
		sleep(waitsec);
		waitsec *= 2;
	}

	if (fr->is_dir) {	// scan a directory recursively
		std::string name = fr->getNextEntry();
		while (!name.empty()) {		// empty name means there's no more entries in the directory
			if (ft == PANGU)
				scan(path + name);
			else
				scan(path + "/" + name);
			name = fr->getNextEntry();
		}
	}
	else {	// scan a file
		pr_msg("size:\t%'16lu\tbytes", fr->size);
		if (fr->size < min_size) {
			pr_msg("skipped");
			fr->close();
			delete fr;
			return;
		}
		if (scan_skip-- > 0) {
			pr_msg("skipped");
			fr->close();
			delete fr;
			return;
		}
		if (scan_count > 0 && total_files >= scan_count) {
			pr_msg("skipped");
			fr->close();
			delete fr;
			return;
		}
		if (meta_only) {	// only look at meta data
			save_file(fr);
			fr->close();
			delete fr;
			return;
		}

		std::vector<Scanner *> scanners;
		if (scan_all) {		// first build up a list of scanners
            scanners.push_back(new Scanner(false, 2048, 0, stream_map[get_logfile_name(false, 2048, false)], stream_mapdata[get_datafile_name(false, 2048, false)]));
            //scanners.push_back(new Scanner(false, 4, 0, stream_map[get_logfile_name(false, 4, false)], stream_mapdata[get_datafile_name(false, 4, false)]));
            scanners.push_back(new Scanner(true, 4, 0, stream_map[get_logfile_name(true, 4, false)],   stream_mapdata[get_datafile_name(true, 4, false)]));
            //scanners.push_back(new Scanner(true, 8, 0, stream_map[get_logfile_name(true, 8, false)],   stream_mapdata[get_datafile_name(true, 8, false)]));
            //scanners.push_back(new Scanner(true, 16, 0, stream_map[get_logfile_name(true, 16, false)], stream_mapdata[get_datafile_name(true, 16, false)]));
            scanners.push_back(new Scanner(true, 4, 0x1 << 21, stream_map[get_logfile_name(true, 4, true)], stream_mapdata[get_datafile_name(true, 4, true)]));
		}
		else {
			scanners.push_back(new Scanner(is_var, block_size, fix_boundary_size,
                                           stream_map[get_logfile_name(is_var, block_size, use_fix_boundary)],
                                           stream_mapdata[get_datafile_name(is_var, block_size, use_fix_boundary)]));
		}
		
		// then feed scanners with data, the whole file's hash is also caculated here
		SHA_CTX ctx;
		bool eof = false;
		uint64_t nbytes = 0, total_bytes = 0;
		std::vector<Scanner *>::iterator it;

		SHA1_Init(&ctx);
		nbytes = load_data(fr, &p1, &p2, &ctx, total_bytes);
		while (nbytes) {	// right now just assume pangu log file can fill up the buffer
			total_bytes += nbytes;
			fprintf(stderr, "\rread:\t%'16llu\tbytes", total_bytes);
			fflush(stderr);
			if (nbytes != READ_BUFFER_SIZE)
				eof = true;
			for (it = scanners.begin(); it != scanners.end(); it++)
				(*it)->addData(p1, p2, nbytes, eof);
			if (eof)
				break;
			nbytes = load_data(fr, &p1, &p2, &ctx, total_bytes);
		}
		SHA1_Final(fr->cksum, &ctx);

		fprintf(stderr, "\nsaving results...");
		fflush(stderr);
		save_file(fr);
		for (it = scanners.begin(); it != scanners.end(); it++) {
			delete (*it);
		}
		pr_msg("done");
	}

	fr->close();
	delete fr;
}

int main(int argc, char **argv)
{
	scan_all = true;
	is_var = true;
	block_size = 4;	// default 4 KB
	ft = LOCAL;
	total_files = 0;
	total_size = 0;
	meta_only = false;
    use_fix_boundary = false;
    fix_boundary_size = 0;
	min_size = 0;
	scan_skip = 0;
	scan_count = 0;

	opterr = 0;
	int option;
	file_id = 0;
	logname = "dedup_result";
    dataname = "";
    dataname.clear();
	path = ".";

	if (argc <= 1) {
		usage(argv[0]);
		return 1;
	}

	while ((option = getopt(argc, argv, "mthb:k:n:s:f:v:p:l:o:d:a:x:")) != -1)
		switch (option) {
        case 'a':
            path = optarg;
            ft = KVFILE;
            break;
        case 'x':
            prefix = optarg;
            break;
		case 'm':
			meta_only = true;
			break;
		case 'k':
			scan_skip = atoi(optarg);
			break;
		case 'n':
			scan_count = atoi(optarg);
			break;
		case 's':
			min_size = atoi(optarg);
			break;
        case 'b':
            scan_all = false;
            use_fix_boundary = true;
            fix_boundary_size = atoi(optarg);
            break;
		case 'f':
			scan_all = false;
			is_var = false;
			block_size = atoi(optarg);
			break;
		case 'v':
			scan_all = false;
			is_var = true;
			block_size = atoi(optarg);
			break;
		case 'p':
			path = optarg;
			ft = PANGU;
			break;
		case 'l':
			path = optarg;
			ft = LOCAL;
			break;
		case 'o':
			logname = optarg;
			break;
        case 'd':
            dataname = optarg;
            break;
		case 't':
			setlocale(LC_ALL,"");	// print readable numbers
			break;
		case 'h':
			usage(argv[0]);
			return 1;
		case '?':
			usage(argv[0]);
			return 1;
		default:
			usage(argv[0]);
			return 1;
		}
#ifdef USE_PANGU
	if (ft == PANGU) {
		apsara::InitApsara(argc, argv);
		apsara::pangu::InitPangu();
		CapabilitySet capability = InternalCapability::Get(InternalCapability::PU, STRING_FLAG(CapabilityFile), 
                                                           string("You should specify internal Capability file by using '--CapabilityFile <CAP_FILE_PATH>'"));
		FileSystem::GetInstance()->SetCapability(capability);
	}

    if (ft == KVFILE) {
		apsara::InitApsara(argc, argv);
		apsara::pangu::InitPangu();
    }
#endif	/* USE_PANGU */

    // now prepare output data streams
	std::ios_base::openmode om = std::ios_base::out | std::ios_base::binary | std::ios_base::trunc;
	stream_map["file"] = new std::ofstream(logname.c_str());
	if (stream_map["file"]->is_open())
		pr_msg("file opened");
	if (meta_only) {
		// do nothing		
	}
	else if (scan_all) {
		stream_map[get_logfile_name(false, 2048, false)] = new std::ofstream(get_logfile_name(false, 2048, false).c_str(), om);
        //stream_map[get_logfile_name(false, 4, false)] = new std::ofstream(get_logfile_name(false, 4, false).c_str(), om);
		stream_map[get_logfile_name(true, 4, false)] = new std::ofstream(get_logfile_name(true, 4, false).c_str(), om);
		//stream_map[get_logfile_name(true, 8, false)] = new std::ofstream(get_logfile_name(true, 8, false).c_str(), om);
		//stream_map[get_logfile_name(true, 16, false)] = new std::ofstream(get_logfile_name(true, 16, false).c_str(), om);
        stream_map[get_logfile_name(true, 4, true)] = new std::ofstream(get_logfile_name(true, 4, true).c_str(), om);
        if(dataname.empty())
        {
            stream_mapdata[get_datafile_name(false, 2048, false)] = NULL;
            //stream_mapdata[get_datafile_name(false, 4, false)] = NULL;
            stream_mapdata[get_datafile_name(true, 4, false)] = NULL;
            //stream_mapdata[get_datafile_name(true, 8, false)] = NULL;
            //stream_mapdata[get_datafile_name(true, 16, false)] = NULL;
            stream_mapdata[get_datafile_name(true, 4, true)] = NULL;
        }
        else{
            stream_mapdata[get_datafile_name(false, 2048, false)] = new std::ofstream(get_datafile_name(false, 2048, false).c_str(), om);
            //stream_mapdata[get_datafile_name(false, 4, false)] = new std::ofstream(get_datafile_name(false, 4, false).c_str(), om);
            stream_mapdata[get_datafile_name(true, 4, false)] = new std::ofstream(get_datafile_name(true, 4, false).c_str(), om);
            //stream_mapdata[get_datafile_name(true, 8, false)] = new std::ofstream(get_datafile_name(true, 8, false).c_str(), om);
            //stream_mapdata[get_datafile_name(true, 16, false)] = new std::ofstream(get_datafile_name(true, 16, false).c_str(), om);
            stream_mapdata[get_datafile_name(true, 4, true)] = new std::ofstream(get_datafile_name(true, 4, true).c_str(), om);
        }
    }
	else{
		stream_map[get_logfile_name(is_var, block_size, use_fix_boundary)] = new ofstream(get_logfile_name(is_var, block_size, use_fix_boundary).c_str(), om);
        if(dataname.empty())
		    stream_mapdata[get_datafile_name(is_var, block_size, use_fix_boundary)] = NULL;
        else
		    stream_mapdata[get_datafile_name(is_var, block_size, use_fix_boundary)] = new ofstream(get_datafile_name(is_var, block_size, use_fix_boundary).c_str(), om);
    }

	scan(path);

	std::map<std::string, std::ofstream *>::iterator it;
	for (it = stream_map.begin(); it != stream_map.end(); it ++) {
		(*it).second->close();
		delete (*it).second;
	}

	pr_msg("%'lu files, %'lu bytes", total_files, total_size);
	return 0;
}
