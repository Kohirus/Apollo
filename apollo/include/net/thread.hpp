#ifndef __APOLLO_THREAD_HPP__
#define __APOLLO_THREAD_HPP__

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>

namespace apollo {
/**
 * @brief 线程封装类
 * 
 */
class Thread {
public:
    using ThreadFunc = std::function<void()>;

    /**
     * @brief Construct a new Thread object
     * 
     * @param func 线程函数
     * @param name 线程名称
     */
    explicit Thread(ThreadFunc func, const std::string& name = std::string());
    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;
    ~Thread();

    /**
     * @brief 启动线程
     * 
     */
    void start();

    /**
     * @brief 等待线程结束
     * 
     */
    void join();

    /**
     * @brief 线程是否已经启动
     *
     */
    bool started() const { return started_; }

    /**
     * @brief 返回线程ID
     * 
     */
    pid_t tid() const { return tid_; }

    /**
     * @brief 返回线程名称
     * 
     */
    const std::string& name() const { return name_; }

    /**
     * @brief 返回所创建的线程的数量
     * 
     */
    static int numCreated() { return numCreated_; }

    /**
     * @brief 获取底层的线程指针
     * 
     * @return const std::thread* 
     */
    std::thread* getThreadPtr() const { return thread_.get(); }

private:
    /**
     * @brief 设置线程的默认名称
     */
    void setDefaultName();

private:
    bool started_; // 线程是否启动
    bool joined_;

    std::shared_ptr<std::thread> thread_; // 线程对象

    pid_t       tid_;  // 线程id
    ThreadFunc  func_; // 线程函数
    std::string name_; // 线程名称

    static std::atomic_int numCreated_;
};
} // namespace apollo

#endif // !__APOLLO_THREAD_HPP__