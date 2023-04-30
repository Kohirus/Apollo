#ifndef __APOLLO_RADIX_TREE_H__
#define __APOLLO_RADIX_TREE_H__

#include "utilis.h"
#include <cassert>
#include <cstring>

namespace apollo {
template <int BITS>
class RadixTree {
public:
    using idx_t = uintptr_t;

private:
    static const int kInteriorBits   = (BITS + 2) / 3;           // 第一、二层对应页号的比特位个数
    static const int kInteriorLength = 1 << kInteriorBits;       // 第一、二层存储元素的个数
    static const int kLeafBits       = BITS - 2 * kInteriorBits; // 第三层对应页号的比特位个数
    static const int kLeafLength     = 1 << kLeafBits;           // 第三层存储元素的个数

    struct Node {
        Node* ptrs_[kInteriorLength];
    };
    struct Leaf {
        void* values_[kLeafLength];
    };

    Node* newNode() {
        static ObjectPool<Node> node_pool;
        Node*                   res = node_pool.alloc();
        if (res != nullptr) {
            memset(res, 0, sizeof(*res));
        }
        return res;
    }

    Node* root_;

public:
    explicit RadixTree() {
        root_ = newNode();
    }

    void* get(idx_t idx) const {
        const idx_t idx_first  = idx >> (kLeafBits + kInteriorBits);         // 第一层对应的下标
        const idx_t idx_second = (idx >> kLeafBits) & (kInteriorLength - 1); // 第二层对应的下标
        const idx_t idx_third  = idx & (kLeafLength - 1);                    // 第三层对应的下标
        // 页号超出范围 或映射该页号的空间未开辟
        if ((idx >> BITS) > 0 || root_->ptrs_[idx_first] == nullptr
            || root_->ptrs_[idx_first]->ptrs_[idx_second] == nullptr) {
            return nullptr;
        }
        return reinterpret_cast<Leaf*>(root_->ptrs_[idx_first]->ptrs_[idx_second])->values_[idx_third];
    }

    void set(idx_t idx, void* ptr) {
        assert(idx >> BITS == 0);
        const idx_t idx_first  = idx >> (kLeafBits + kInteriorBits);         // 第一层对应的下标
        const idx_t idx_second = (idx >> kLeafBits) & (kInteriorLength - 1); // 第二层对应的下标
        const idx_t idx_third  = idx & (kLeafLength - 1);                    // 第三层对应的下标
        ensure(idx, 1);                                                      // 确保映射第_idx页页号的空间是开辟好了的
        // 建立该页号与对应span的映射
        reinterpret_cast<Leaf*>(root_->ptrs_[idx_first]->ptrs_[idx_second])->values_[idx_third] = ptr;
    }

private:
    /**
     * @brief 确保[_start, _start+_n-1]页号的空间是开辟好的
     */
    bool ensure(idx_t start, size_t n) {
        for (idx_t key = start; key <= start + n - 1;) {
            const idx_t idx_first  = key >> (kLeafBits + kInteriorBits);         // 第一层对应的下标
            const idx_t idx_second = (key >> kLeafBits) & (kInteriorLength - 1); // 第二层对应的下标
            // 下标值超出范围
            if (idx_first >= kInteriorLength || idx_second >= kInteriorLength)
                return false;
            if (root_->ptrs_[idx_first] == nullptr) // 第一层idx_first下标指向的空间未开辟
            {
                // 开辟对应空间
                Node* n = newNode();
                if (n == nullptr)
                    return false;
                root_->ptrs_[idx_first] = n;
            }
            if (root_->ptrs_[idx_first]->ptrs_[idx_second] == nullptr) // 第二层idx_second下标指向的空间未开辟
            {
                // 开辟对应空间
                static ObjectPool<Leaf> leaf_pool;
                Leaf*                   leaf = leaf_pool.alloc();
                if (leaf == nullptr)
                    return false;
                memset(leaf, 0, sizeof(*leaf));
                root_->ptrs_[idx_first]->ptrs_[idx_second] = reinterpret_cast<Node*>(leaf);
            }
            key = ((key >> kLeafBits) + 1) << kLeafBits; // 继续后续检查
        }
        return true;
    }
};

#ifdef _WIN64
using PageMap = RadixTree<64 - kPageShift>;
#elif _WIN32
using PageMap = RadixTree<32 - kPageShift>;
#elif __linux__
using PageMap = RadixTree<64 - kPageShift>;
#endif

} // namespace apollo

#endif // !__APOLLO_RADIX_TREE_H__
