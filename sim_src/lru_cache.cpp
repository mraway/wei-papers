#include <queue>
#include "lru_cache.hpp"

bool TimerNewer(const LruCacheEntry& left, const LruCacheEntry& right)
{
    //pr_msg("%lu %lu %lu %lu", left.latime_.tv_sec, left.latime_.tv_usec,
    //right.latime_.tv_sec, right.latime_.tv_usec);
    return (timercmp(&(left.latime_), &(right.latime_), <=) == 0);
}

LruCacheEntry::LruCacheEntry(const Block& blk, const struct timeval& tv) : Block(blk), latime_(tv)
{
}

void LruCacheEntry::SetValue(const Block& blk, const struct timeval& tv)
{
    Block::SetValue(blk);
    latime_ = tv;
}

void LruCacheEntry::SetAccessTime(const struct timeval& tv)
{
    latime_ = tv;
}

LruCache::LruCache(int size)
{
    cache_.reserve(size);
    capacity_ = size;
}

void LruCache::RemoveItems(int n)
{
    std::sort(cache_.begin(), cache_.end(), TimerNewer);
    for (int i = 0; i < n && !cache_.empty(); i ++)
        cache_.pop_back();
}

void LruCache::AddItems(const Segment& seg)
{
    struct timeval tv;
    struct timezone tz;
    LruCacheEntry ent;
    gettimeofday(&tv, &tz);

    if (cache_.size() >= capacity_) {	// just replace elements
        std::sort(cache_.begin(), cache_.end(), TimerNewer);
        for (uint32_t i = 0; i < seg.blocklist_.size(); i ++) {
            cache_[cache_.size() - i - 1].SetValue(seg.blocklist_[i], tv);
        }
    }
    else {
        // make some room for the new data
        if (seg.blocklist_.size() + cache_.size() > capacity_) {
            RemoveItems(seg.blocklist_.size() + cache_.size() - capacity_);
        }
        for (uint32_t i = 0; i < seg.blocklist_.size(); i ++) {
            ent.SetValue(seg.blocklist_[i], tv);
            cache_.push_back(ent);
        }
    }
    // sort by hash
    std::sort(cache_.begin(), cache_.end());
}

/*
void LruCache::AddSegment(const Segment& seg)
{

    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    LruCacheEntry ent;

    if (cache_.size() >= capacity_) {
        pr_msg("full");
        std::sort(cache_.begin(), cache_.end(), TimerNewer);
        for (uint32_t i = 0; i < seg.blocklist_.size(); i ++) {
            cache_[cache_.size() - i - 1].SetValue(seg.blocklist_[i], tv);
        }
    }
    else {
        pr_msg("not full");
        for (uint32_t i = 0; i < seg.blocklist_.size(); i ++) {
            ent.SetValue(seg.blocklist_[i], tv);
            cache_.push_back(ent);
        }
    }

    // sort by hash
    std::sort(cache_.begin(), cache_.end());
    pr_msg("add done");
}
*/
bool LruCache::SearchItem(const Block& blk)
{
    LruCacheEntry ent;
    struct timeval tv;
    struct timezone tz;
    std::vector<LruCacheEntry>::iterator it;
    gettimeofday(&tv, &tz);
    ent.SetValue(blk, tv);
    it = std::lower_bound(cache_.begin(), cache_.end(), ent);
    if (it != cache_.end() && (*it) == ent) {
        (*it).SetAccessTime(tv);
        return true;
    }
    return false;
}

bool TimeComparison::operator()(const CacheIterator &left, const CacheIterator &right) const
{
    return timercmp(&(*left).second, &(*right).second, <) > 0;
}

CdsLruCache::CdsLruCache(int size)
{
    capacity_ = size;
}

bool CdsLruCache::SearchItem(const Block &blk)
{
    std::map<Block, struct timeval>::iterator it = cache_.find(blk);
    if (it == cache_.end())
        return false;

    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    (*it).second = tv;	// update the timestamp if found
    return true;
}

void CdsLruCache::AddItem(const Block &blk)
{
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    cache_[blk] = tv;

    if (cache_.size() > capacity_)
        RemoveItems(capacity_ / 10);
}

void CdsLruCache::BatchAddItem(const std::vector<Block> &blklist)
{
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    for (size_t i = 0; i < blklist.size(); i ++)
        cache_[blklist[i]] = tv;

    while (cache_.size() > capacity_)
        RemoveItems(cache_.size() - capacity_ + capacity_ / 10);
}

void CdsLruCache::RemoveItems(size_t n)
{
    std::priority_queue<CacheIterator, std::vector<CacheIterator>, TimeComparison> myheap;
    for (CacheIterator it = cache_.begin(); it != cache_.end(); it ++)
    {
        // heap not full, just insert
        if (myheap.size() < n) {
            myheap.push(it);
            continue;
        }
        // heap full, remove the top if necessary
        if (timercmp(&(*it).second, &(*myheap.top()).second, <) > 0) {
            myheap.pop();
            myheap.push(it);
        }
    }

    std::vector<CacheIterator> *myvector;
    myvector = reinterpret_cast<std::vector<CacheIterator> *>(&myheap);
    for (std::vector<CacheIterator>::iterator it = myvector->begin(); it != myvector->end(); it ++)
    {
        cache_.erase(*it);
    }
}

void CdsLruCache::Save(std::ostream &os)
{
    for (CacheIterator it = cache_.begin(); it != cache_.end(); it++)
        it->first.Save(os);
}

void CdsLruCache::Load(std::istream &is)
{
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);

    Block blk;
    while (blk.Load(is))
        cache_[blk] = tv;
}




















