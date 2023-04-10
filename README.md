# Apollo

一款高性能的后端分布式服务器框架。

## 项目要点

- 使用 C++11 重写并简化 [muduo](https://github.com/chenshuo/muduo) 网络库；
- 仿写了 [tcmalloc](https://github.com/google/tcmalloc) 的高并发内存池；
- 加入了 [sylar](https://github.com/sylar-yin/sylar) 的日志模块并将其改进为异步日志；
- 使用到了 protobuf 和 Zookeeper 来作为分布式协调服务；
- 使用 Nginx 来处理集群行为；