/**
 * @file main.cpp
 * @author Leaos
 * @brief 内存池测试
 * @version 0.1
 * @date 2023-02-23
 * 
 * @copyright Copyright (c) 2023
 */

#include <iostream>
#include <libgen.h>
#include "benchmark.h"
#include "concurrentalloc.h"
using namespace std;
using namespace apollo;

int main(int argc, char* argv[]) {

#ifdef TCMALLOC
    cout << "defined!" << endl;
#else
    cout << "undefined!" << endl;
#endif

    if (argc <= 2) {
        cout << "Usage: " << basename(argv[0]) << " malloc_free_count thread_count" << endl;
        return 0;
    }
    cout << "=============" << endl;
    size_t n         = atoi(argv[1]);
    size_t threadcnt = atoi(argv[2]);
    BenchmarkMalloc(n, threadcnt, 10);
    cout << endl
         << endl;
    BenchmarkConcurrentMalloc(n, threadcnt, 10);
    cout << "=============" << endl;
    return 0;
}
