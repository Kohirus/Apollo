#include "bytebuffer.hpp"
#include "log.hpp"
#include <cassert>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
using namespace apollo;

ByteBuffer::ByteBuffer(uint32_t size)
    : capacity_(size)
    , length_(0)
    , head_(0)
    , data_(nullptr) {
    data_ = new char[size];
    assert(data_);
}

ByteBuffer::ByteBuffer(const ByteBuffer& other)
    : capacity_(other.capacity_)
    , head_(other.head_)
    , length_(other.length_)
    , data_(nullptr) {
    data_ = new char[capacity_];
    assert(data_);
    memcpy(data_, other.data_, capacity_);
}

ByteBuffer& ByteBuffer::operator=(const ByteBuffer& other) {
    if (this != &other) {
        copy(other);
    }
    return *this;
}

ByteBuffer::~ByteBuffer() {
    delete[] data_;
    data_     = nullptr;
    capacity_ = 0;
    length_   = 0;
    head_     = 0;
}

void ByteBuffer::clear() {
    length_ = head_ = 0;
}

void ByteBuffer::compact() {
    if (head_ != 0) {
        if (length_ != 0) {
            memmove(data_, data_ + head_, length_);
        }
        head_ = 0;
    }
}

void ByteBuffer::copy(const ByteBuffer& other) {
    delete[] data_;
    data_ = new char[other.capacity_];
    assert(data_);
    memcpy(data_, other.data_, other.capacity_);
    head_     = other.head_;
    length_   = other.length_;
    capacity_ = other.capacity_;
}

void ByteBuffer::checkSize(uint32_t size) {
    if (size <= remaining()) {
        // 剩余空间足以存放size字节 直接返回
        return;
    }

    if (size <= capacity_ - length_) {
        // 无效数据空间足以存放 size字节 压缩有效数据即可
        compact();
        return;
    }

    uint32_t new_size = capacity_ + (size + kIncreaseSize - 1) / kIncreaseSize * kIncreaseSize;
    char*    new_buf  = new char[new_size];
    assert(new_buf);
    memcpy(new_buf, begin(), length_);

    head_     = 0;
    capacity_ = new_size;
}

void ByteBuffer::pop(uint32_t len) {
    assert(len <= length_);
    length_ -= len;
    head_ += len;
}

int ByteBuffer::readFd(int fd) {
    int need_read = 0;

    // 一次性读出所有的数据 需要给fd设置FIONREAD
    if (ioctl(fd, FIONREAD, &need_read) == -1) {
        LOG_ERROR(g_logger) << "Failed to ioctl FIONREAD";
        return -1;
    }

    checkSize(need_read);
    int already_read = 0;

    do {
        if (need_read == 0) {
            already_read = read(fd, end(), remaining());
        } else {
            already_read = read(fd, end(), need_read);
        }
    } while (already_read == -1 && errno == EINTR);

    if (already_read > 0) {
        if (need_read != 0) {
            assert(already_read == need_read);
        }
        length_ += already_read;
    }

    return already_read;
}

const char* ByteBuffer::data() const {
    return data_ + head_;
}

void ByteBuffer::append(const char* data, uint32_t len) {
    checkSize(len);

    memcpy(end(), data, len);
    length_ += len;
}

int ByteBuffer::writeFd(int fd) {
    int already_write = 0;

    do {
        already_write = write(fd, begin(), length_);
    } while (already_write == -1 && errno == EINTR);

    if (already_write > 0) {
        pop(already_write);
        compact();
    }

    if (already_write == -1 && errno == EAGAIN) {
        already_write = 0;
    }

    return already_write;
}