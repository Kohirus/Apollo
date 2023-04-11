#ifndef __APOLLO_CHANNEL_HPP__
#define __APOLLO_CHANNEL_HPP__

#include "timestamp.hpp"
#include <functional>
#include <memory>

namespace apollo {

class EventLoop;

/**
 * @brief 通道类
 * @details 封装了sockfd和其感兴趣的event，例如EPOLLIN、EPOLLOUT事件
 * 还绑定了Poller返回的具体的事件
 */
class Channel {
public:
    using ReadEventCallback = std::function<void(Timestamp)>;
    using EventCallback     = std::function<void()>;

    Channel(EventLoop* loop, int fd);
    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;
    ~Channel();

    /**
     * @brief fd得到相应事件后处理事件
     *
     * @param reveiveTime
     */
    void handleEvent(Timestamp reveiveTime);

    /**
     * @brief 设置读事件的回调函数
     *
     * @param cb
     */
    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }

    /**
     * @brief 设置写事件的回调函数
     * 
     * @param cb 
     */
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }

    /**
     * @brief 设置对端关闭事件的回调函数
     * 
     * @param cb 
     */
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }

    /**
     * @brief 设置错误事件的回调函数
     * 
     * @param cb 
     */
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    /**
     * @brief 防止当Channel被手动remove掉，Channel还在执行回调操作
     * 
     */
    void tie(const std::shared_ptr<void>&);

    /**
     * @brief 返回Channel所绑定的套接字描述符
     * 
     * @return int 
     */
    int fd() const { return fd_; }

    /**
     * @brief 返回fd所感兴趣的事件
     * 
     * @return int 
     */
    int events() const { return events_; }

    /**
     * @brief 设置fd上实际发生的事件
     * 
     * @param ev 
     */
    void setRevents(int ev) { revents_ = ev; }

    /**
     * @brief 当前fd上是否没有发生任何事件
     * 
     */
    bool isNoneEvent() const { return events_ == kNoneEvent; }

    /**
     * @brief 当前fd上是否正在发生读事件
     * 
     */
    bool isReading() const { return events_ == kReadEvent; }

    /**
     * @brief 当前fd上是否正在发生写事件
     * 
     */
    bool isWriting() const { return events_ == kWriteEvent; }

    /**
     * @brief 开启fd上的读事件
     * 
     */
    void enableReading();

    /**
     * @brief 关闭fd上的读事件
     * 
     */
    void disableReading();

    /**
     * @brief 开启fd上的写事件
     * 
     */
    void enableWriting();

    /**
     * @brief 关闭fd上的写事件
     * 
     */
    void disableWriting();

    /**
     * @brief 关闭fd上所有的事件
     * 
     */
    void disableAll();

    int index() const { return index_; }

    void setIndex(int idx) { index_ = idx; }

    /**
     * @brief 返回Channel所属的事件循环
     * 
     * @return EventLoop* 
     */
    EventLoop* ownerLoop() const { return loop_; }

    /**
     * @brief 将当前Channel从所属的事件循环中删除
     * 
     */
    void remove();

private:
    /**
     * @brief 让Poller更新fd上所感兴趣的事件
     * 
     */
    void update();

    /**
     * @brief 安全的处理相应事件 
     * 
     * @param receiveTime 
     */
    void handleEventWithGurad(Timestamp receiveTime);

private:
    static const int kNoneEvent;  // 没有任何事件
    static const int kReadEvent;  // 读事件
    static const int kWriteEvent; // 写事件

    EventLoop* loop_; // 事件循环
    const int  fd_;   // Poller监听的套接字描述符对象

    int events_;  // fd感兴趣的事件
    int revents_; // fd实际发生的具体的事件
    int index_;

    std::weak_ptr<void> tie_; // 防止Channel被手动remove
    bool                tied_;

    // 由于Channel通道中能够获知fd最终发生的具体的时间
    // 因此由Channel来负责调用具体的事件回调操作

    ReadEventCallback readCallback_;  // 读事件回调
    EventCallback     writeCallback_; // 写事件回调
    EventCallback     closeCallback_; // 关闭事件回调
    EventCallback     errorCallback_; // 错误事件回调
};
} // namespace apollo

#endif // !__APOLLO_CHANNEL_HPP__