#include "threadcache.hpp"
#include "centralcache.hpp"
using namespace apollo;

void* ThreadCache::allocate(size_t size) {
    assert(size <= kMaxBytes);

    size_t align = AlignHelper::roundUp(size);
    size_t index = AlignHelper::index(size);

    if (!freelists_[index].empty()) {
        return freelists_[index].pop();
    } else {
        return fetchFromCentralCache(index, align);
    }
}

void ThreadCache::deallocate(void* ptr, size_t size) {
    assert(ptr && size <= kMaxBytes);

    // 找出对应的自由链表桶将对象插入
    size_t index = AlignHelper::index(size);
    freelists_[index].push(ptr);

    // 当自由链表长度大于一次批量申请的对象个数时就开始还一段list给CentralCache
    if (freelists_[index].size() >= freelists_[index].maxSize()) {
        revertListToCentralCache(freelists_[index], size);
    }
}

void* ThreadCache::fetchFromCentralCache(size_t index, size_t size) {
    // 慢开始反馈调节算法
    // 刚开始不会一次向CentralCache申请太多对象，因为太多了会造成内存浪费
    // 如果不断有_size大小的需求，那么fetchnum就会不断增长 直到达到上限
    size_t limit    = freelists_[index].maxSize();
    size_t fetchnum = std::min(limit, AlignHelper::numMoveSize(size));
    if (fetchnum == limit) {
        freelists_[index].setMaxSize(limit + 1);
    }

    void * start = nullptr, *end = nullptr;
    size_t actualnum = CentralCache::getInstance()->fetchRangeObj(start, end, fetchnum, size);
    assert(actualnum >= 1); // 至少有一个对象

    if (actualnum == 1) // 申请到对象的个数是一个，则直接将这一个对象返回即可
    {
        assert(start == end);
        return start;
    } else // 申请到对象的个数是多个，还需要将剩下的对象挂到ThreadCache中对应的哈希桶中
    {
        freelists_[index].pushRange(nextObj(start), end, actualnum - 1);
        return start;
    }
}

void ThreadCache::revertListToCentralCache(FreeList& list, size_t size) {
    // void* start = nullptr, * end = nullptr;
    //// 从list中取出一次批量个数的对象
    //_list.PopRange(start, end, _list.MaxSize());
    void* start = list.clear();

    // 将取出的对象还给CentralCache中对应的Span
    CentralCache::getInstance()->releaseList(start, size);
}
