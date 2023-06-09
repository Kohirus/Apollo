#ifndef __APOLLO_THREAD_CACHE_H__
#define __APOLLO_THREAD_CACHE_H__

#include "utilis.h"

namespace apollo {
/**
 * @brief 线程缓存对象
 * @details 线程独享，无需锁变量。其允许申请的最大内存为256KB
 */
class ThreadCache {
public:
    ThreadCache()                              = default;
    ThreadCache(const ThreadCache&)            = delete;
    ThreadCache& operator=(const ThreadCache&) = delete;

    /**
     * @brief 申请内存对象
     */
    void* allocate(size_t size);

    /**
     * @brief 释放内存对象
     *
     * @param _ptr 要释放的内存对象
     * @param _size 对象的大小
     */
    void deallocate(void* ptr, size_t size);

private:
    /**
     * @brief 向CentralCache申请对象
     *
     * @param _index 对象在哈希桶中的索引
     * @param _size 对象的大小
     */
    void* fetchFromCentralCache(size_t index, size_t size);

    /**
     * @brief 将FreeList对象归还给CentralCache
     *
     * @param _list 要归还的自由链表
     * @param _size 自由链表中内存块的大小
     */
    void revertListToCentralCache(FreeList& list, size_t size);

private:
    FreeList freelists_[kBucketSize];
};
} // namespace apollo

#endif // !__APOLLO_THREAD_CACHE_H__
