#include "cellbuffer.hpp"
#include "concurrentalloc.hpp"
#include <cstring>
#include <iostream>
using namespace apollo;

CellBuffer::CellBuffer(size_t len)
    : status(BufferStatus::FREE)
    , prev(nullptr)
    , next(nullptr)
    , totalLen_(len)
    , usedLen_(0) {
    data_ = new char[len];
    if (!data_) {
        std::cerr << "No space to allocate data_" << std::endl;
        exit(1);
    }
}

size_t CellBuffer::availLen() const {
    return totalLen_ - usedLen_;
}

bool CellBuffer::empty() const {
    return usedLen_ == 0;
}

void CellBuffer::append(const char* context, size_t len) {
    assert(len <= availLen());
    memcpy(data_ + usedLen_, context, len);
    usedLen_ += len;
}

void CellBuffer::clear() {
    usedLen_ = 0;
    status  = BufferStatus::FREE;
}
