#ifndef __UTILTS_HPP__
#define __UTILTS_HPP__

#include <new>
#include <cassert>
#include <mutex>

#ifdef _WIN32
#include <Windows.h>
#elif __linux__
#include <unistd.h>
#endif

namespace apollo {

/// ThreadCache申请内存的上限 即256KB
static const size_t kMaxBytes = 256 * 1024;
/// 哈希桶的数目
static const size_t kBucketSize = 208;
/// PageCache中哈希桶的数目
static const size_t kPageBucketSize = 129;
/// 页大小偏移转换 即2的12次方为4096 即一页的大小
static const size_t kPageShift = 12;

/// 页号: 地址=页号*4K
#ifdef _WIN64
using page_t = unsigned long long;
#elif _WIN32
using page_t = size_t;
#elif __linux__
using page_t = unsigned long long;
#endif

/// 线程局部存储
#ifdef _WIN32
#define TLS __declspec(thread)
#elif __linux__
#define TLS __thread
#endif

/**
 * @brief 调用系统接口申请内存
 */
inline static void* systemAlloc(size_t npage) {
#ifdef _WIN32
    void* ptr = VirtualAlloc(0, _npage * (1 << kPageShift), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#elif __linux__
    void* ptr = sbrk(npage * (1 << kPageShift));
#endif
    if (ptr == nullptr) {
        throw std::bad_alloc();
    }
    return ptr;
}

/**
 * @brief 调用系统接口释放内存
 */
inline static void systemFree(void* ptr) {
#ifdef _WIN32
    VirtualFree(_ptr, 0, MEM_RELEASE);
#elif __linux__
    brk(ptr);
#endif
}

/**
 * @breif 访问下一个对象
 */
inline static void*& nextObj(void* ptr) {
    return (*(void**)ptr);
}

/**
 * @brief 用于字节对齐和哈希映射
 * @details 不同字节数的对齐数不同
 *  ======================================
 *   字节数          对齐数        哈希桶下标
 *  ======================================
 *	[1,128]           8           [0, 16)
 *	[129, 1024]       16          [16, 72)
 *	[1025, 8K]        128         [72, 128)
 *	[8K+1, 64K]       1024        [128, 184)
 *	[64K+1, 256K]     8K          [184, 208)
 *  ======================================
 */
class AlignHelper {
public:
    /**
     * @brief 获取向上对齐后的字节数
     */
    static inline size_t roundUp(size_t bytes) {
        if (bytes <= 128) {
            return _roundUp(bytes, 8);
        } else if (bytes <= 1024) {
            return _roundUp(bytes, 16);
        } else if (bytes <= 8 * 1024) {
            return _roundUp(bytes, 128);
        } else if (bytes <= 64 * 1024) {
            return _roundUp(bytes, 1024);
        } else if (bytes <= 256 * 1024) {
            return _roundUp(bytes, 8 * 1024);
        } else {
            // 大于256KB的按页对齐
            return _roundUp(bytes, 1 << kPageShift);
        }
    }

    /**
     * @brief 获取对应的哈希桶的下标
     */
    static inline size_t index(size_t bytes) {
        // 每个区间内的自由链表数目
        static size_t group[4] = { 16, 56, 56, 56 };

        if (bytes <= 128) {
            return _index(bytes, 3);
        } else if (bytes <= 1024) {
            return _index(bytes - 128, 4) + group[0];
        } else if (bytes <= 8 * 1024) {
            return _index(bytes - 1024, 7) + group[0] + group[1];
        } else if (bytes <= 64 * 1024) {
            return _index(bytes - 8 * 1024, 10) + group[0] + group[1] + group[2];
        } else if (bytes <= 256 * 1024) {
            return _index(bytes - 64 * 1024, 13) + group[0] + group[1] + group[2] + group[3];
        } else {
            assert(false);
            return -1;
        }
    }

    /**
     * @brief 获取CentralCache实际应给ThreadCache的具体的对象个数
     * @details 通过慢开始反馈调节算法，将对象数目控制在2~512个之间
     */
    static size_t numMoveSize(size_t size) {
        assert(size > 0);

        // 对象越小，计算出的上限越高
        // 对象越大，计算出的上限越低
        int num = kMaxBytes / size;
        if (num < 2)
            num = 2;
        if (num > 512)
            num = 512;

        return num;
    }

    static size_t numMovePage(size_t size) {
        size_t num = numMoveSize(size); // 计算出ThreadCache一次向CentralCache申请对象的个数上限
        // 先计算num个size大小的对象所需的字节数 然后将字节数转化为页数
        size_t npage = (num * size) >> kPageShift;

        if (npage == 0) // 至少给一页
        {
            npage = 1;
        }

        return npage;
    }

private:
    /**
     * @brief 获取向上对齐后的字节数
     *
     * @param _bytes 需要调整的字节数
     * @param _align 对齐数
     * @return 返回对齐后的字节数
     */
    static inline size_t _roundUp(size_t bytes, size_t align) {
        return ((bytes + align - 1) & ~(align - 1));
    }

    /**
     * @brief 获取对应的哈希桶的下标
     *
     * @param _bytes 字节数
     * @param _align 将对齐数转换为2的n次方的形式 例如对齐数为8 则传入3
     * @return 返回字节数对应的哈希桶下标
     */
    static inline size_t _index(size_t bytes, size_t alignShift) {
        return ((bytes + (1 << alignShift) - 1) >> alignShift) - 1;
    }
};

/**
 * @brief 管理小内存块的自由链表
 */
class FreeList {
public:
    FreeList()
        : freelist_(nullptr)
        , maxSize_(1)
        , size_(0) { }

    /**
     * @brief 将对象头插到自由链表中
     */
    void push(void* obj) {
        assert(obj);

        nextObj(obj) = freelist_;
        freelist_     = obj;
    }

    /**
     * @brief 弹出自由链表的头部对象
     */
    void* pop() {
        assert(freelist_);

        void* obj = freelist_;
        freelist_ = nextObj(freelist_);
        return obj;
    }

    /**
     * @brief 将start和end之间的对象插入到自由链表中
     *
     * @param _start 所插入对象的起始地址
     * @param _end 所插入对象的结束地址
     * @param _cnt 所插入的对象的个数
     */
    void pushRange(void* start, void* end, size_t cnt) {
        assert(start);
        assert(end);

        // 头插
        nextObj(end) = freelist_;
        freelist_     = start;
        size_ += cnt;
    }

    /**
     * @brief 将start和end之间的对象从自由链表中移除
     *
     * @param _start 传出参数，所移除对象的起始地址
     * @param _end 传出参数，所移除对象的结束地址
     * @param _cnt 所移除对象的个数
     */
    void popRange(void*& start, void*& end, size_t cnt) {
        assert(cnt <= size_);

        // 头删
        start = freelist_;
        end   = start;
        for (size_t i = 0; i < cnt - 1; i++) {
            end = nextObj(end);
        }
        freelist_     = nextObj(end); // 自由链表指向end的下一个对象
        nextObj(end) = nullptr;       // 取出的一段链表的表尾置空
        size_ -= cnt;
    }

    void* clear() {
        size_      = 0;
        void* list = freelist_;
        freelist_  = nullptr;
        return list;
    }

    bool   empty() const { return freelist_ == nullptr; }
    size_t maxSize() const { return maxSize_; }
    void   setMaxSize(size_t _size) { maxSize_ = _size; }
    size_t size() const { return size_; }

private:
    void*  freelist_;
    size_t maxSize_; // 内存块的最大数目
    size_t size_;
};

/**
 * @brief 对象池
 */
template <class T>
class ObjectPool {
public:
    /// 分配内存
    T* alloc() {
        T* obj = nullptr;

        // 优先利用归还的内存块对象
        if (freelist_ != nullptr) {
            // 从自由链表头部删除一个对象
            obj       = (T*)freelist_;
            freelist_ = nextObj(freelist_);
        } else {
            // 保证对象足够存储地址
            size_t objsize = sizeof(T) < sizeof(void*) ? sizeof(void*) : sizeof(T);
            // 剩余内存不够一个对象大小时 重新开辟大块内存空间
            if (remainBytes_ < objsize) {
                // 如果对象大小大于默认大小则需要进行更改
                size_t bytes = sizeof(T) > kMemorySize_ ? sizeof(T) : kMemorySize_;
                memory_      = (char*)systemAlloc(bytes >> kPageShift);
                if (memory_ == nullptr) {
                    throw std::bad_alloc();
                }
                remainBytes_ = bytes;
            }
            // 从大块内存中切出objsize字节的内存
            obj = (T*)memory_;
            memory_ += objsize;
            remainBytes_ -= objsize;
        }
        // 定位new 显式调用对象的构造函数
        new (obj) T;

        return obj;
    }

    /// 释放内存
    void free(T* obj) {
        // 显式调用析构函数清理对象
        obj->~T();

        // 将释放的对象头插到自由链表
        nextObj(obj) = freelist_;
        freelist_     = obj;
    }

private:
    char*  memory_      = nullptr; // 指向大块内存
    size_t remainBytes_ = 0;       // 大块内存在切分过程中剩余的字节数
    void*  freelist_    = nullptr; // 内存块归还后形成的自由链表

    const size_t kMemorySize_ = 128 * 1024;
};

/**
 * @brief 管理以页为单位的大内存块
 */
struct Span {
    Span()
        : pageId_(0)
        , cnt_(0)
        , next_(nullptr)
        , prev_(nullptr)
        , useCnt_(0)
        , freelist_(nullptr)
        , used_(false)
        , blockSize_(0) { }

    page_t pageId_;    // 大块内存的起始页号
    size_t cnt_;       // 页的数量
    Span*  next_;      // 下一个大块内存
    Span*  prev_;      // 上一个大块内存
    size_t useCnt_;    // 切割为小块内存后，分配给ThreadCache的计数
    void*  freelist_;  // 切割为小块内存后形成的自由链表
    bool   used_;      // 是否正在被使用
    size_t blockSize_; // 所切割的小内存块的大小
};

/**
 * @brief 带有头节点的大内存块双向链表
 */
class SpanList {
public:
    SpanList() {
        head_        = spanpool_.alloc();
        head_->next_ = head_;
        head_->prev_ = head_;
    }

    /**
     * @brief 将newspan插入到pos之前
     *
     * @param _pos 要插入的位置
     * @param _newspan 被插入的对象
     */
    void insert(Span* pos, Span* newspan) {
        assert(pos && newspan);

        Span* prev = pos->prev_;

        prev->next_     = newspan;
        newspan->prev_ = prev;

        newspan->next_ = pos;
        pos->prev_     = newspan;
    }

    /**
     * @brief 将pos指向的元素从双向链表中移除
     */
    void erase(Span* pos) {
        assert(pos && pos != head_);

        Span* prev = pos->prev_;
        Span* next = pos->next_;

        prev->next_ = next;
        next->prev_ = prev;
    }

    Span* begin() { return head_->next_; }
    Span* end() { return head_; }
    bool  empty() { return head_ == head_->next_; }

    void  pushFront(Span* span) { insert(begin(), span); }
    Span* popFront() {
        Span* front = head_->next_;
        erase(front);
        return front;
    }

private:
    Span*                   head_;
    static ObjectPool<Span> spanpool_;

public:
    std::mutex mtx_;
};

}

#endif // !__UTILTS_HPP__
