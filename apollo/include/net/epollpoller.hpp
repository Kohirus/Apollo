#ifndef __APOLLO_EPOLLPOLLER_HPP__
#define __APOLLO_EPOLLPOLLER_HPP__

#include "poller.hpp"
#include <sys/epoll.h>

namespace apollo {

/**
 * @brief Epoll多路复用模型
 * 
 */
class EPollPoller : public Poller {
public:
    EPollPoller(EventLoop* loop);
    ~EPollPoller() override;

    /**
     * @brief 重写父类poll事件，阻塞等待激活事件的到来
     * 
     * @param timeoutMs 超时事件
     * @param activeChannels 传出参数，激活的事件列表 
     * @return Timestamp 返回激活事件到来的时间戳
     */
    Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;

    /**
     * @brief 重写父类事件，更新Channel对象上的事件
     * 
     * @param channel 
     */
    void updateChannel(Channel* channel) override;

    /**
     * @brief 重写父类事件，移除指定的Channel对象
     * 
     * @param channel 
     */
    void removeChannel(Channel* channel) override;

private:
    /**
     * @brief 填充活跃的连接
     * 
     * @param numEvents 连接数目
     * @param activeChannels 传出参数，激活的事件列表
     */
    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;

    /**
     * @brief 更新Channel对象上的事件
     * 
     * @param operation 操作类型
     * @param channel Channel对象
     */
    void update(int operation, Channel* channel);

private:
    static const int kInitEventListSize = 16; // EventList初始化长度

    using EventList = std::vector<epoll_event>;

    int       epollfd_; // 由epoll返回的描述符
    EventList events_;  // 用于epoll_wait接收活跃事件
};
} // namespace apollo

#endif // __APOLLO_EPOLLPOLLER_HPP__