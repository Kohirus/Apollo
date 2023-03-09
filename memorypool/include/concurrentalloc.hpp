#ifndef __CONCURRENT_ALLOC_HPP__
#define __CONCURRENT_ALLOC_HPP__

#include "threadcache.hpp"
#include "pagecache.hpp"
#include <cassert>

void* operator new(size_t size);
void* operator new[](size_t size);
void operator delete(void* ptr);
void operator delete[](void* ptr);

namespace apollo {

static void* concurrentAlloc(size_t size) {
    if (size > kMaxBytes) // 大于256KB的内存申请
    {
        // 计算出对齐后需要申请的页数
        size_t alignsize = AlignHelper::roundUp(size);
        size_t npage     = alignsize >> kPageShift;

        // 向PageCache申请npage页的span
        Span* span = nullptr;
        {
            PageCache*                  cache = PageCache::getInstance();
            std::lock_guard<std::mutex> lock(cache->mtx_);
            span             = cache->newSpan(npage);
            span->used_      = true;
            span->blockSize_ = size;
        }
        assert(span);

        void* ptr = (void*)(span->pageId_ << kPageShift);
        return ptr;
    } else {
        // 通过TLS，每个线程无锁的获取自己专属的ThreadCache对象
        if (s_tlsthreadcache == nullptr) {
            static std::mutex              mtx;
            static ObjectPool<ThreadCache> tcpool;
            {
                std::lock_guard<std::mutex> lock(mtx);
                s_tlsthreadcache = tcpool.alloc();
            }
        }

        return s_tlsthreadcache->allocate(size);
    }
}

static void concurrentFree(void* ptr) {
    if (ptr != nullptr) {
        Span*  span = PageCache::getInstance()->mapToSpan(ptr);
        size_t size = span->blockSize_;

        if (size > kMaxBytes) // 大于256KB的内存释放
        {
            PageCache* cache = PageCache::getInstance();

            std::lock_guard<std::mutex> lock(cache->mtx_);

            cache->revertSpanToPageCache(span);
        } else {
            assert(s_tlsthreadcache);
            s_tlsthreadcache->deallocate(ptr, size);
        }
    }
}
}

#endif // !__CONCURRENT_ALLOC_HPP__
