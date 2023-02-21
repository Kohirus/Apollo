#ifndef __CELL_BUFFER_HPP__
#define __CELL_BUFFER_HPP__

#include <stddef.h>

namespace apollo {
class CellBuffer {
public:
    /// 缓冲区状态
    enum BufferStatus {
        FREE, // 未满
        FULL  // 已满
    };

    CellBuffer(size_t len);
    CellBuffer(const CellBuffer&)            = delete;
    CellBuffer& operator=(const CellBuffer&) = delete;

    /**
     * @brief 返回缓冲区剩余可用长度
     */
    size_t availLen() const;
    /**
     * @brief 缓冲区是否为空
     */
    bool empty() const;
    /**
     * @brief 向缓冲区中追加内容
     *
     * @param _context 要追加的内容
     * @param _len 追加内容的长度
     */
    void append(const char* context, size_t len);
    /**
     * @brief 清空缓冲区内容
     */
    void clear();

    BufferStatus status; // 缓冲区状态
    CellBuffer*  prev;   // 上一个缓冲区
    CellBuffer*  next;   // 下一个缓冲区

private:
    char*  data_;     // 缓冲区起始地址
    size_t totalLen_; // 缓冲区总长度
    size_t usedLen_;  // 已使用的长度
};
}

#endif // !__CELL_BUFFER_HPP__
