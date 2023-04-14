#ifndef __APOLLO_BUFFER_HPP__
#define __APOLLO_BUFFER_HPP__

#include <string>
#include <vector>

namespace apollo {
/**
 * @brief 读写缓冲区
 * 
 */
class Buffer {
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize  = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize)
        , readerIndex_(kCheapPrepend)
        , writerIndex_(kCheapPrepend) { }
    Buffer(const Buffer&) = default;
    Buffer& operator=(const Buffer&) = default;
    ~Buffer()                        = default;

    /**
     * @brief 返回可读的字节数
     * 
     * @return size_t 
     */
    size_t readableBytes() const { return writerIndex_ - readerIndex_; }

    /**
     * @brief 返回可写的字节数
     * 
     * @return size_t 
     */
    size_t writeableBytes() const { return buffer_.size() - writerIndex_; }

    /**
     * @brief 返回预留的字节数
     * 
     * @return size_t 
     */
    size_t prependabelBytes() const { return readerIndex_; }

    /**
     * @brief 返回缓冲区中可读数据的起始地址
     * 
     * @return const char* 
     */
    const char* peek() const { return begin() + readerIndex_; }

    /**
     * @brief 读取长度为len的数据后移动读指针
     * 
     * @param len 
     */
    void retrieve(size_t len);

    /**
     * @brief 可读数据读完，恢复读指针和写指针
     * 
     */
    void retrieveAll();

    /**
     * @brief 将缓冲区数据以std::string形式返回
     * 
     * @return std::string 
     */
    std::string retrieveAllAsString();

    /**
     * @brief 将缓冲区中长度为len的数据以std::string形式返回
     * 
     * @param len 数据长度
     * @return std::string 
     */
    std::string retrieveAsString(size_t len);

    /**
     * @brief 确保可写区域至少有len个字节长度
     * 
     * @param len 
     */
    void ensureWritableBytes(size_t len);

    /**
     * @brief 向缓冲区中追加数据
     * 
     * @param data 数据起始地址
     * @param len 数据长度
     */
    void append(const char* data, size_t len);

    /**
     * @brief 向缓冲区中追加数据
     * 
     * @param data 
     */
    void append(const std::string& data);

    /**
     * @brief 从fd中读取数据到缓冲区
     * 
     * @param fd 文件描述符
     * @param saveErrno 传出参数，保存错误号
     * @return ssize_t 返回读取到的数据长度
     */
    ssize_t readFd(int fd, int& saveErrno);

    /**
     * @brief 将缓冲区中的可读数据写入到fd中
     * 
     * @param fd 文件描述符
     * @param saveErrno 传出参数，保存错误号
     * @return ssize_t 返回写入的数据长度 
     */
    ssize_t writeFd(int fd, int& saveErrno);

    /**
     * @brief 与目标缓冲区进行交换
     * 
     * @param rhs 
     */
    void swap(Buffer& rhs);

private:
    /**
     * @brief 返回缓冲区的起始地址
     * 
     * @return char* 
     */
    char* begin() { return &*buffer_.begin(); }

    /**
     * @brief 返回缓冲区的起始地址
     * 
     * @return const char* 
     */
    const char* begin() const { return &*buffer_.begin(); }

    /**
     * @brief 返回写起始指针位置
     * 
     * @return char* 
     */
    char* beginWrite() { return begin() + writerIndex_; }

    /**
     * @brief 返回写起始指针位置
     * 
     * @return const char* 
     */
    const char* beginWrite() const { return begin() + writerIndex_; }

    /**
     * @brief 扩容到至少有len字节
     * 
     * @param len 
     */
    void makeSpace(size_t len);

private:
    std::vector<char> buffer_;      // 动态缓冲区
    size_t            readerIndex_; // 读指针
    size_t            writerIndex_; // 写指针
};
} // namespace apollo

#endif // __APOLLO_BUFFER_HPP__