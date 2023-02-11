#include "pagecache.hpp"
#include <cassert>
using namespace apollo;

Span* PageCache::newSpan(size_t npage) {
    assert(npage > 0);

    if (npage > kPageBucketSize - 1) // 大于128页直接找堆申请
    {
        void* ptr     = systemAlloc(npage);
        Span* span    = span_pool_.alloc();
        span->pageId_ = (page_t)ptr >> kPageShift;
        span->cnt_    = npage;
        // 建立页号与span之间的映射
        // hash_[span->page_id_] = span;
        hash_.set(span->pageId_, span);
        return span;
    }

    // 先检查第_npage个桶里面有没有span 有则直接返回
    if (!spanlists_[npage].empty()) {
        Span* res = spanlists_[npage].popFront();

        // 建立页号与span的映射，方便CentralCache回收小块内存时查找对应的Span
        for (page_t i = 0; i < res->cnt_; i++) {
            // hash_[res->page_id_ + i] = res;
            hash_.set(res->pageId_ + i, res);
        }

        return res;
    }

    // 如果没有则检查一下后面的桶里面有没有span 如果有可以将其进行切分
    for (size_t i = npage + 1; i < kPageBucketSize; i++) {
        if (!spanlists_[i].empty()) {
            Span* nSpan = spanlists_[i].popFront();
            Span* kSpan = span_pool_.alloc();

            // 在nSpan的头部切k页下来
            kSpan->pageId_ = nSpan->pageId_;
            kSpan->cnt_    = npage;

            nSpan->pageId_ += npage;
            nSpan->cnt_ -= npage;
            // 将剩下的挂到对应映射的位置
            spanlists_[nSpan->cnt_].pushFront(nSpan);
            // 存储nSpan的首尾页号与nSpan之间的映射，方便PageCache合并span时进行前后页的查找
            // hash_[nSpan->page_id_] = nSpan;
            // hash_[nSpan->page_id_ + nSpan->cnt_ - 1] = nSpan;
            hash_.set(nSpan->pageId_, nSpan);
            hash_.set(nSpan->pageId_ + nSpan->cnt_ - 1, nSpan);

            // 建立页号与span的映射，方便CentralCache回收小块内存时查找对应的Span
            for (page_t i = 0; i < kSpan->cnt_; i++) {
                // hash_[kSpan->page_id_ + i] = kSpan;
                hash_.set(kSpan->pageId_ + i, kSpan);
            }

            return kSpan;
        }
    }

    // 走到这里说明后面没有大页的span了，这时就向堆申请一个128页的span
    Span* largespan    = span_pool_.alloc();
    void* ptr          = systemAlloc(kPageBucketSize - 1);
    largespan->pageId_ = (page_t)ptr >> kPageShift;
    largespan->cnt_    = kPageBucketSize - 1;

    spanlists_[largespan->cnt_].pushFront(largespan);

    // 尽量避免代码重复，递归调用自己
    return newSpan(npage);
}

Span* PageCache::mapToSpan(void* obj) {
    page_t id = (page_t)obj >> kPageShift; // 获取页号

    // std::lock_guard<std::mutex> lock(mtx_);

    // auto ret = hash_.find(id);
    // if (ret != hash_.end())
    //{
    //	return ret->second;
    // }
    // else
    //{
    //	assert(false);
    //	return nullptr;
    // }

    Span* ret = (Span*)hash_.get(id);
    assert(ret);
    return ret;
}

void PageCache::revertSpanToPageCache(Span* span) {
    if (span->cnt_ > kPageBucketSize - 1) // 大于128页直接释放给堆
    {
        void* ptr = (void*)(span->pageId_ << kPageShift);
        systemFree(ptr);
        span_pool_.free(span);
        return;
    }

    // 对span的前后页，尝试进行合并，缓解内存碎片问题
    // 向前合并
    while (1) {
        page_t prev_id = span->pageId_ - 1;
        // auto ret = hash_.find(prev_id);
        //  前面的页号没有（还未向系统申请），停止向前合并
        // if (ret == hash_.end())
        //{
        //	break;
        // }
        Span* ret = (Span*)hash_.get(prev_id);
        if (ret == nullptr)
            break;

        // 前面的页号对应的span正在被使用，停止向前合并
        // Span* prev_span = ret->second;
        Span* prev_span = ret;
        if (prev_span->used_ == true) {
            break;
        }

        // 合并出超过128页的span无法进行管理，停止向前合并
        if (prev_span->cnt_ + span->cnt_ > kPageBucketSize - 1) {
            break;
        }

        // 进行向前合并
        span->pageId_ = prev_span->pageId_;
        span->cnt_ += prev_span->cnt_;

        // 将prevSpan从对应的双链表中移除
        spanlists_[prev_span->cnt_].erase(prev_span);

        span_pool_.free(prev_span);
    }

    // 向后合并
    while (1) {
        page_t next_id = span->pageId_ + span->cnt_;
        // auto ret = hash_.find(next_id);
        //// 后面的页号没有（还未向系统申请），停止向后合并
        // if (ret == hash_.end())
        //{
        //	break;
        // }
        Span* ret = (Span*)hash_.get(next_id);
        if (ret == nullptr)
            break;

        // 后面的页号对应的span正在被使用，停止向后合并
        // Span* next_span = ret->second;
        Span* next_span = ret;
        if (next_span->used_ == true) {
            break;
        }

        // 合并出超过128页的span无法进行管理，停止向后合并
        if (next_span->cnt_ + span->cnt_ > kPageBucketSize - 1) {
            break;
        }

        // 进行向后合并
        span->cnt_ += next_span->cnt_;

        // 将nextSpan从对应的双链表中移除
        spanlists_[next_span->cnt_].erase(next_span);

        span_pool_.free(next_span);
    }

    // 将合并后的span挂到对应的双链表当中
    spanlists_[span->cnt_].pushFront(span);
    // 建立该span与其首尾页的映射
    // hash_[span->page_id_] = span;
    // hash_[span->page_id_ + span->cnt_ - 1] = span;
    hash_.set(span->pageId_, span);
    hash_.set(span->pageId_ + span->cnt_ - 1, span);

    // 将该span设置为未被使用的状态
    span->used_ = false;
}
