#ifndef __PAGE_CACHE_HPP__
#define __PAGE_CACHE_HPP__

#include "utilis.hpp"
#include "radixtree.hpp"
#include <mutex>
// #include <unordered_map>

namespace apollo {
/**
 * @brief 页缓存对象
 * @details 线程共享，需要对象锁
 */
class PageCache {
public:
    static PageCache* getInstance() {
        static PageCache cache;
        return &cache;
    }

    /**
     * @brief 获取一个_npage页的Span对象
     */
    Span* newSpan(size_t npage);

    /**
     * @brief 获取对象到Span的映射
     */
    Span* mapToSpan(void* obj);

    /**
     * @brief 释放空闲的Span到PageCache 并合并相邻的Span
     */
    void revertSpanToPageCache(Span* span);

private:
    PageCache()                            = default;
    PageCache(const PageCache&)            = delete;
    PageCache& operator=(const PageCache&) = delete;

public:
    std::mutex mtx_;

private:
    SpanList spanlists_[kPageBucketSize];
    // std::unordered_map<page_t, Span*> hash_;
    PageMap          hash_;
    ObjectPool<Span> span_pool_;
};
}

#endif // !__PAGE_CACHE_HPP__
