#ifndef __BYTE_BUFFER_HPP__
#define __BYTE_BUFFER_HPP__

#include <stdint.h>
#include <string>

namespace apollo {

/**
 * @brief 缓冲区, 非线程安全
 */
class ByteBuffer {
public:
    ByteBuffer(uint32_t size = kBufferSize);
    ByteBuffer(const ByteBuffer& other);
    ByteBuffer& operator=(const ByteBuffer& other);
    ~ByteBuffer();

    /**
     * @brief 返回有效数据的有效长度
     */
    uint32_t length() const { return length_; }

    /**
     * @brief 返回有效数据的起始索引
     */
    void* begin() const { return (void*)(data_ + head_); }

    /**
     * @brief 返回有效数据的末尾索引
     */
    void* end() const { return (void*)(data_ + head_ + length_); }

    /**
     * @brief 返回剩余空间的大小
     */
    uint32_t remaining() const { return capacity_ - (head_ + length_); }

    /**
     * @brief 清空数据
     */
    void clear();

    /**
     * @brief 压缩数据
     * @details 清空已处理的数据, 将未处理的数据提前至数据首地址
     */
    void compact();

    /**
     * @brief 拷贝ByteBuffer数据
     *
     * @param other 要拷贝的对象
     */
    void copy(const ByteBuffer& other);

    /**
     * @brief 处理长度为len的数据
     *
     * @param len 要处理数据的长度
     */
    void pop(uint32_t len);

    /**
     * @brief 从文件描述符中读取数据到缓冲区中
     *
     * @param fd 要读取的文件描述符
     * @return int 返回值小于0时表示错误, 大于0时表示实际读取到的数据长度
     */
    int readFd(int fd);

    /**
     * @brief 取出读取到的有效数据
     *
     * @return const char*
     */
    const char* data() const;

    /**
     * @brief 向缓冲区追加数据
     *
     * @param data 数据首地址
     * @param len 数据长度
     */
    void append(const char* data, uint32_t len);

    /**
     * @brief 向缓冲区追加数据
     *
     * @param data 要追加的数据
     */
    void append(const std::string& data) { append(data.c_str(), data.size()); }

    /**
     * @brief 将缓冲区中的数据写入到文件描述符中
     *
     * @param fd 要写入的文件描述符
     * @return int 返回写入数据的真实长度
     */
    int writeFd(int fd);

private:
    /**
     * @brief 检查剩余的空间大小, 如果空间不足则进行扩容
     *
     * @param size 需要的内存大小
     */
    void checkSize(uint32_t size);

private:
    static const uint32_t kBufferSize   = 2048; // 默认的缓冲区大小
    static const uint32_t kIncreaseSize = 2048; // 缓冲区扩大时的增长因子

    char*    data_;     // 数据地址
    uint32_t capacity_; // 缓冲区容量大小
    uint32_t length_;   // 有效数据长度
    uint32_t head_;     // 未处理数据的头部索引位置
};

}

#endif // !__BYTE_BUFFER_HPP__