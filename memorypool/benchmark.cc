#include "benchmark.h"
#include "concurrentalloc.h"
#include "utilis.h"
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>
using namespace std;
using namespace apollo;

void BenchmarkMalloc(size_t ntimes, size_t nworks, size_t rounds) {
    std::vector<std::thread> vthread(nworks);
    std::atomic<size_t>      malloc_costtime;
    malloc_costtime = 0;
    std::atomic<size_t> free_costtime;
    free_costtime = 0;
    for (size_t k = 0; k < nworks; ++k) {
        vthread[k] = std::thread([&]() {
            std::vector<void*> v;
            v.reserve(ntimes);
            for (size_t j = 0; j < rounds; ++j) {
                size_t begin1 = clock();
                for (size_t i = 0; i < ntimes; i++) {
                    // v.push_back(malloc(16));
                    v.push_back(malloc((16 + i) % 8192 + 1));
                }
                size_t end1   = clock();
                size_t begin2 = clock();
                for (size_t i = 0; i < ntimes; i++) {
                    free(v[i]);
                }
                size_t end2 = clock();
                v.clear();
                malloc_costtime += (end1 - begin1);
                free_costtime += (end2 - begin2);
            }
        });
    }
    for (auto& t : vthread) {
        t.join();
    }

    cout << nworks << " 个线程并发执行 " << rounds << " 轮次，每轮次 malloc " << ntimes
         << " 次，共花费 " << (double)malloc_costtime / CLOCKS_PER_SEC << " s" << endl;
    cout << nworks << " 个线程并发执行 " << rounds << " 轮次，每轮次 free " << ntimes
         << " 次，共花费 " << (double)free_costtime / CLOCKS_PER_SEC << " s" << endl;
    cout << nworks << " 个线程并发执行 malloc&free " << nworks * rounds * ntimes
         << " 次，共花费 " << ((double)malloc_costtime + (double)free_costtime) / CLOCKS_PER_SEC << " s" << endl;
}

void BenchmarkConcurrentMalloc(size_t ntimes, size_t nworks, size_t rounds) {
    std::vector<std::thread> vthread(nworks);
    std::atomic<size_t>      malloc_costtime;
    malloc_costtime = 0;
    std::atomic<size_t> free_costtime;
    free_costtime = 0;
    for (size_t k = 0; k < nworks; ++k) {
        vthread[k] = std::thread([&]() {
            std::vector<void*> v;
            v.reserve(ntimes);
            for (size_t j = 0; j < rounds; ++j) {
                size_t begin1 = clock();
                for (size_t i = 0; i < ntimes; i++) {
                    // v.push_back(ConcurrentAlloc(16));
                    v.push_back(concurrentAlloc((16 + i) % 8192 + 1));
                }
                size_t end1   = clock();
                size_t begin2 = clock();
                for (size_t i = 0; i < ntimes; i++) {
                    concurrentFree(v[i]);
                }
                size_t end2 = clock();
                v.clear();
                malloc_costtime += (end1 - begin1);
                free_costtime += (end2 - begin2);
            }
        });
    }
    for (auto& t : vthread) {
        t.join();
    }

    cout << nworks << " 个线程并发执行 " << rounds << " 轮次，每轮次 alloc " << ntimes
         << " 次，共花费 " << (double)malloc_costtime / CLOCKS_PER_SEC << " s" << endl;
    cout << nworks << " 个线程并发执行 " << rounds << " 轮次，每轮次 dealloc " << ntimes
         << " 次，共花费 " << (double)free_costtime / CLOCKS_PER_SEC << " s" << endl;
    cout << nworks << " 个线程并发执行 alloc&dealloc " << nworks * rounds * ntimes
         << " 次，共花费 " << ((double)malloc_costtime + (free_costtime)) / CLOCKS_PER_SEC << " s" << endl;
}
