#ifndef _TEST_H_
#define _TEST_H_

#include <stddef.h>

struct TreeNode {
    int       _val;
    TreeNode* _left;
    TreeNode* _right;
    TreeNode()
        : _val(0)
        , _left(nullptr)
        , _right(nullptr) { }
};

/// 测试malloc/free
void BenchmarkMalloc(size_t ntimes, size_t nworks, size_t rounds);
/// 测试concurrentAlloc/concurrentFree
void BenchmarkConcurrentMalloc(size_t ntimes, size_t nworks, size_t rounds);

#endif // !_TEST_H_
