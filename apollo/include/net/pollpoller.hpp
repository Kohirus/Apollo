#ifndef __APOLLO_POLLPOLLER_HPP__
#define __APOLLO_POLLPOLLER_HPP__

#include "poller.hpp"
#include <poll.h>

namespace apollo {
class PollPoller : public Poller {
public:
    PollPoller(EventLoop* loop);
    ~PollPoller() override = default;

    /**
     * @brief 重写父类方法，阻塞等待激活事件的到来
     * 
     * @param timeoutMs 超时时间
     * @param activeChannels 传出参数，激活的时间列表 
     * @return Timestamp 返回激活事件到来的时间戳
     */
    Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;

    /**
     * @brief 重写父类方法，更新Channel对象上的事件
     * 
     * @param channel 
     */
    void updateChannel(Channel* channel) override;

    /**
     * @brief 重写父类方法，将Channel对象从poller中移除
     * 
     * @param channel 
     */
    void removeChannel(Channel* channel) override;

private:
    /**
     * @brief 填充活跃的事件列表
     * 
     * @param numEvents 活跃事件的数量
     * @param activeChannels 传出参数，活跃事件列表
     */
    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;

private:
    using PollFdList = std::vector<pollfd>;

    PollFdList pollfds_;
};
} // namespace apollo

#endif // !__APOLLO_POLLPOLLER_HPP__