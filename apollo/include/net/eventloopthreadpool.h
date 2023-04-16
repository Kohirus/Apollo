#ifndef __APOLLO_EVENTLOOPTHREADPOOL_H__
#define __APOLLO_EVENTLOOPTHREADPOOL_H__

#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace apollo {

class EventLoop;
class EventLoopThread;

/**
 * @brief 事件循环线程池
 * 
 */
class EventLoopThreadPool {
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    /**
     * @brief Construct a new Event Loop Thread Pool object
     * 
     * @param baseLoop MainLoop
     * @param nameArg 线程池名称
     */
    EventLoopThreadPool(EventLoop* baseLoop, const std::string& nameArg);
    EventLoopThreadPool(const EventLoopThreadPool&) = delete;
    EventLoopThreadPool& operator=(const EventLoopThreadPool&) = delete;
    ~EventLoopThreadPool();

    /**
     * @brief 设置线程数量
     * 
     * @param numThreads 线程数量
     */
    void setThreadNum(int numThreads);

    /**
     * @brief 启动线程池
     * 
     * @param cb 线程初始化函数
     */
    void start(const ThreadInitCallback& cb = ThreadInitCallback());

    /**
     * @brief 获取下一个事件循环
     * @details 当工作在多线程环境下时，默认以轮询的方式访问SubLoop
     * 
     * @return EventLoop* 
     */
    EventLoop* getNextLoop();

    /**
     * @brief 获取所有的事件循环
     * 
     * @return std::vector<EventLoop*> 
     */
    std::vector<EventLoop*> getAllLoop() const;

    /**
     * @brief 线程池是否已经启动
     * 
     */
    bool started() const { return started_; }

    /**
     * @brief 返回线程池名称
     * 
     * @return const std::string& 
     */
    const std::string& name() const { return name_; }

private:
    EventLoop*  mainLoop_;   // 基本事件循环
    std::string name_;       // 线程池名称
    bool        started_;    // 线程池是否启动
    int         numThreads_; // 线程数量
    int         next_;       // 下一个执行的事件循环

    std::vector<std::unique_ptr<EventLoopThread>> threads_; // 线程对象

    std::vector<EventLoop*> loops_; // 事件循环对象
};
} // namespace apollo

#endif // !__APOLLO_EVENTLOOPTHREADPOOL_H__