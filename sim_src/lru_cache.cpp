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

LruCache::LruCache(int size = DEFAULT_CACHE_SIZE)
{
    cache_.reserve(size);
    capacity_ = size;
}

void LruCache::RemoveEntries(int n)
{
    std::sort(cache_.begin(), cache_.end(), TimerNewer);
    for (int i = 0; i < n && !cache_.empty(); i ++)
        cache_.pop_back();
}

void LruCache::AddSegment(const Segment& seg)
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
            RemoveEntries(seg.blocklist_.size() + cache_.size() - capacity_);
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
bool LruCache::SearchEntry(const Block& blk)
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
