#ifndef _LRU_CACHE_H_
#define _LRU_CACHE_H_

#include <vector>
#include <map>
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
    void AddItems(const Segment& seg);

    bool SearchItem(const Block& blk);

private:
    /*
     * discard the n oldest entries from cache
     */
    void RemoveItems(int n);
    std::vector<LruCacheEntry> cache_;
    uint32_t capacity_;
};


typedef std::map<Block, struct timeval>::iterator CacheIterator;

class TimeComparison
{
public:
    bool operator() (const CacheIterator& left, const CacheIterator& right) const;
};

class CdsLruCache
{
public:
    CdsLruCache(int size = DEFAULT_CACHE_SIZE);

    /*
     * add an item to cache
     * if cache size exceeds the capacity, then remove 10% of oldest items
     */
    void AddItem(const Block& blk);

    void BatchAddItem(const std::vector<Block>& blklist);

    bool SearchItem(const Block& blk);

    /*
     * save and load cache contents
     * timestamp is ignored
     */
    void Save(std::ostream& os);

    void Load(std::istream& is);

private:
    void RemoveItems(size_t n);
    
    std::map<Block, struct timeval> cache_;

    uint32_t capacity_;
};



#endif
