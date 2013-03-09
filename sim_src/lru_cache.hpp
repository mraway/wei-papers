#ifndef _LRU_CACHE_H_
#define _LRU_CACHE_H_

#include <vector>
#include <sys/time.h>
#include "dedup_types.hpp"

#define DEFAULT_CACHE_SIZE 512

class LruCacheEntry : public Block
{
    /*
     * to sort the entries by access time, from new to old
     * this comparison shall return true if left is newer than right
     */
    friend bool TimerNewer(const LruCacheEntry& left, const LruCacheEntry& right);
public:
    LruCacheEntry() {}

    LruCacheEntry(const Block& blk, const struct timeval& tv);

    void SetValue(const Block& blk, const struct timeval& tv);

    void SetAccessTime(const struct timeval& tv);
private:
    struct timeval latime_;
};
    

class LruCache 
{
public:
    LruCache(int size);

    /*
     * add all the blocks into cache, discard old entries by timestamp
     */
    void AddSegment(const Segment& seg);

    bool SearchEntry(const Block& blk);

private:
    /*
     * discard the n oldest entries from cache
     */
    void RemoveEntries(int n);
    std::vector<LruCacheEntry> cache_;
    uint32_t capacity_;
};

#endif
