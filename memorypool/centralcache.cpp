#include "centralcache.hpp"
#include "pagecache.hpp"
using namespace apollo;

size_t CentralCache::fetchRangeObj(void*& start, void*& end, size_t cnt, size_t size) {
    size_t                       index = AlignHelper::index(size);
    std::unique_lock<std::mutex> lock(spanlists_[index].mtx_); // 加锁

    // 在对应的哈希桶中获取一个非空的span
    Span* span = getOneSpan(spanlists_[index], size, lock);

    // span不为空且其自由链表也不为空
    assert(span && span->freelist_);

    // 从span中获取n个对象 如果不够n个，有多少拿多少
    start           = span->freelist_;
    end             = span->freelist_;
    size_t actualnum = 1;
    while (nextObj(end) && (cnt - 1)) {
        end = nextObj(end);
        actualnum++;
        cnt--;
    }
    span->freelist_ = nextObj(end); // 取完后剩下的对象继续放到自由链表
    nextObj(end)   = nullptr;       // 取出的一段链表的表尾置空
    span->useCnt_ += actualnum;      // 更新被分配给ThreadCache的计数

    return actualnum;
}

void CentralCache::releaseList(void* start, size_t size) {
    size_t index = AlignHelper::index(size);

    std::unique_lock<std::mutex> bucket_lock(spanlists_[index].mtx_); // 加锁

    while (start) {
        void* next = nextObj(start); // 记录下一个
        Span* span = PageCache::getInstance()->mapToSpan(start);
        // 将对象头插到Span的自由链表
        nextObj(start) = span->freelist_;
        span->freelist_ = start;

        span->useCnt_--;        // 更新被分配给ThreadCache的计数
        if (span->useCnt_ == 0) // 说明这个span分配出去的对象全部都回来了
        {
            // 此时这个span就可以再回收给PageCache，PageCache可以再尝试去做前后页的合并
            spanlists_[index].erase(span);
            span->freelist_ = nullptr; // 自由链表置空
            span->next_     = nullptr;
            span->prev_     = nullptr;

            // 释放span给PageCache时，使用PageCache的锁就可以了，这时把桶锁解掉
            bucket_lock.unlock(); // 解桶锁
            {
                PageCache*                  cache = PageCache::getInstance();
                std::lock_guard<std::mutex> lock(cache->mtx_);
                cache->revertSpanToPageCache(span);
            }

            bucket_lock.lock(); // 加桶锁
        }

        start = next;
    }
}

Span* CentralCache::getOneSpan(SpanList& list, size_t size, std::unique_lock<std::mutex>& bucket_lock) {
    // 先在_list中寻找非空的span 如果有则直接返回
    Span* it = list.begin();
    while (it != list.end()) {
        if (it->freelist_ != nullptr) {
            return it;
        } else {
            it = it->next_;
        }
    }

    bucket_lock.unlock(); // 解桶锁

    Span* span = nullptr;
    {
        std::lock_guard<std::mutex> lock(PageCache::getInstance()->mtx_);

        // 如果_list中没有非空的span，只能向PageCache申请
        span             = PageCache::getInstance()->newSpan(AlignHelper::numMovePage(size));
        span->used_      = true;
        span->blockSize_ = size;
    }
    assert(span);

    // 计算span的大块内存的起始地址和大块内存的大小（字节数）
    char*  start = (char*)(span->pageId_ << kPageShift);
    size_t bytes = span->cnt_ << kPageShift;

    // 把大块内存切成size大小的对象链接起来
    char* end = start + bytes;
    // 先切一块下来去做尾，方便尾插
    span->freelist_ = start;
    start += size;
    void* tail = span->freelist_;
    // 尾插
    while (start < end) {
        nextObj(tail) = start;
        tail          = nextObj(tail);
        start += size;
    }
    nextObj(tail) = nullptr; // 尾的指向置空

    bucket_lock.lock(); // 加桶锁

    // 将切好的span头插到_list
    list.pushFront(span);

    return span;
}
