#include "concurrentalloc.h"
using namespace apollo;

void* operator new(size_t size) {
#ifdef TCMALLOC
    return apollo::concurrentAlloc(size);
#else
    return malloc(size);
#endif
}

void* operator new[](size_t size) {
#ifdef TCMALLOC
    return apollo::concurrentAlloc(size);
#else
    return malloc(size);
#endif
}

void operator delete(void* ptr) noexcept {
#ifdef TCMALLOC
    return apollo::concurrentFree(ptr);
#else
    return free(ptr);
#endif
}

void operator delete[](void* ptr) noexcept {
#ifdef TCMALLOC
    return apollo::concurrentFree(ptr);
#else
    return free(ptr);
#endif
}
