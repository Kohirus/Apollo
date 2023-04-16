#include "thread.h"
#include "common.h"
#include <semaphore.h>
using namespace apollo;

std::atomic_int Thread::numCreated_(0);

Thread::Thread(ThreadFunc func, const std::string& name)
    : started_(false)
    , joined_(false)
    , tid_(0)
    , func_(std::move(func))
    , name_(name) {
    setDefaultName();
}

Thread::~Thread() {
    if (started_ && !joined_) {
        thread_->detach();
    }
}

void Thread::start() {
    started_ = true;
    sem_t sem;
    sem_init(&sem, false, 0);

    thread_ = std::shared_ptr<std::thread>(new std::thread([&]() {
        // 获取线程的tid值
        tid_ = ThreadHelper::ThreadId();
        sem_post(&sem);
        func_();
    }));

    // 等待上方线程获取其tid
    sem_wait(&sem);
}

void Thread::join() {
    joined_ = true;
    thread_->join();
}

void Thread::setDefaultName() {
    int num = ++numCreated_;
    if (name_.empty()) {
        char buf[32] = { 0 };
        snprintf(buf, sizeof(buf), "Thread%d", num);
        name_ = buf;
    }
}