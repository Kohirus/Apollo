#include "buffer.h"
#include <sys/uio.h>
#include <unistd.h>
using namespace apollo;

void Buffer::retrieve(size_t len) {
    if (len < readableBytes()) {
        readerIndex_ += len;
    } else {
        retrieveAll();
    }
}

void Buffer::retrieveAll() {
    readerIndex_ = writerIndex_ = kCheapPrepend;
}

std::string Buffer::retrieveAllAsString() {
    return retrieveAsString(readableBytes());
}

std::string Buffer::retrieveAsString(size_t len) {
    std::string result(peek(), len);
    retrieve(len);
    return result;
}

void Buffer::ensureWritableBytes(size_t len) {
    if (writeableBytes() < len) {
        makeSpace(len);
    }
}

void Buffer::append(const char* data, size_t len) {
    ensureWritableBytes(len);
    std::copy(data, data + len, beginWrite());
    writerIndex_ += len;
}

void Buffer::append(const std::string& data) {
    append(data.c_str(), data.size());
}

ssize_t Buffer::readFd(int fd, int& saveErrno) {
    char  extrabuf[65536] = { 0 };
    iovec vec[2];

    const size_t writeable = writeableBytes();

    vec[0].iov_base = beginWrite();
    vec[0].iov_len  = writeable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len  = sizeof(extrabuf);

    const int     iovcnt = (writeable < sizeof(extrabuf)) ? 2 : 1;
    const ssize_t n      = ::readv(fd, vec, iovcnt);
    if (n < 0) {
        saveErrno = errno;
    } else if (n <= static_cast<ssize_t>(writeable)) {
        writerIndex_ += n;
    } else {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writeable);
    }

    return n;
}

ssize_t Buffer::writeFd(int fd, int& saveErrno) {
    ssize_t n = ::write(fd, peek(), readableBytes());
    if (n < 0) {
        saveErrno = errno;
    }
    return n;
}

void Buffer::swap(Buffer& rhs) {
    buffer_.swap(rhs.buffer_);
    std::swap(readerIndex_, rhs.readerIndex_);
    std::swap(writerIndex_, rhs.writerIndex_);
}

void Buffer::makeSpace(size_t len) {
    if (writeableBytes() + prependabelBytes() - kCheapPrepend < len) {
        // 如果空闲区域不满足长度大小 则扩容
        buffer_.resize(writerIndex_ + len);
    } else {
        size_t readbale = readableBytes();
        std::copy(begin() + readerIndex_,
            begin() + writerIndex_,
            begin() + kCheapPrepend);
        readerIndex_ = kCheapPrepend;
        writerIndex_ = readerIndex_ + readbale;
    }
}