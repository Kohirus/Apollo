# Apollo

使用 C/C++ 开发的一款高性能的后端分布式服务器网络框架。

## 项目要点

- 使用 C++11 重写并简化 [muduo](https://github.com/chenshuo/muduo) 网络库；
- 仿写了 [tcmalloc](https://github.com/google/tcmalloc) 的高并发内存池；
- 加入了 [sylar](https://github.com/sylar-yin/sylar) 的日志模块并将其改进为异步日志；
- 使用到了 protobuf 和 Zookeeper 来作为分布式协调服务；
- 使用 Nginx 来处理集群行为；

## 内存池模块

该项目所使用的内存池原型是 Google 的开源项目 tcmalloc，其全称为 Thread-Caching Malloc，即线程缓存的 malloc，实现了高效的多线程内存管理，用于替换系统的内存分配相关函数，即 malloc 和 free。

内存池主要解决的就是效率问题，它能够避免让程序频繁的向系统申请和释放内存。其次，内存池作为系统的内存分配器，还需要尝试解决内存碎片的问题，内存碎片分为如下两种：

- **外部碎片**：指的是空闲的小块内存区域，由于这些内存空间不连续，以至于合计的内存足够，但是不能满足一些内存分配的需求。
- **内部碎片**：指的是由于一些对齐的需求，导致分配出去的空间中一些内存无法被充分利用。

> 内存池尝试解决的是外部碎片的问题，同时也尽可能的减少内部碎片的产生。

该内存池的整体架构如下图所示：

<div align="center">
<img src="./screenshot/mempool.png" style="zoom:60%"/>
</div>

其主要由以下三个部分组成：

- `Thread Cache`: 线程缓存是每个线程独有的，用于小于等于 256KB 的内存分配，每个线程独享一个 ThreaCache了。
- `Central Cache`: 中心缓存是所有线程共享的，当 ThreadCache 需要内存时会按需从 CentralCache 中获取内存，而当 ThreadCache 中的内存满足一定条件时，CentralCache 也会在合适的时机对其进行回收。
- `Page Cache`: 页缓存中存储的内存是以页为单位进行存储及分配的，当 CentralCache 需要内存时，PageCache 会分配出一定数量的页给 CentralCache，而当 CentralCache 中的内存满足一定条件时，PageCache 也会在合适的时机对其进行回收，并将回收的内存尽可能的进行合并，组成更大的连续内存块，缓解内存碎片的问题。

上述三个部分的主要作用如下：

- Thread Cache: 主要解决锁竞争的问题；
- Central Cache: 主要负责居中调度的问题；
- Page Cache: 主要负责提供以页为单位的大块内存；

### 1.ThreadCache

Thread Cache 的结构如下图所示：

<div align="center">
<img src="./screenshot/threadcache.png" style="zoom:60%"/>
</div>

通过使用**字节对齐**的方法来减少哈希桶的数目，并且进一步增加内存利用率，在设计时，让不同的范围的字节数按照不同的对齐数进行对齐，具体的对齐方式如下：

| 字节数                 | 对齐数 | 哈希桶下标 | 自由链表数目 |
| ---------------------- | ------ | ---------- | ------------ |
| \[1, 128]               | 8      | [0, 16)    | 16           |
| \[129, 1024]            | 16     | [16, 72)   | 56           |
| \[1025, 8\*1024]         | 128    | [72, 128)  | 56           |
| \[8\*1024+1, 64\*1024]   | 1024   | [128, 184) | 56           |
| \[64\*1024+1, 256\*1024] | 8\*1024 | [184, 208) | 24           |

为了实现每个线程无锁访问属于自己的 Thread Cache，就需要用到**线程局部存储**(Thread Local Storage, TLS)，使用该存储方法的变量在它所在的线程是全局可访问的，但是不能被其它线程访问到，这样就保证了数据的线程独立性。

当某个线程申请的对象不用了，可以将其释放给 Thread Cache，然后 Thread Cache 将该对象插入到哈希桶的自由链表当中即可。

但是随着线程不断地释放，对应自由链表中的长度也会越来越长，这些内存堆积在一个 Thread Cache 中就是一种浪费，此时应该将这些内存还给 Central Cache，这样一来，这些内存对于其它线程来说就是可申请的，因此当 Thread Cache 中某个桶当中的自由链表太长时，可以将其释放给 Central Cache。

### 2. CentralCache

当线程申请某一大小的内存时，如果 Thread Cache 中对应的自由链表不为空，那么直接取出一个内存块返回即可，但如果此时该自由链表为空，那么这时 Thread Cache 就需要向 Central Cache 申请内存了。

Central Cache 的结构与 Thread Cache 是一样的，都是哈希桶结构，并且所遵循的对齐规则也一致。这样做的好处是当 Thread Cache 的某个桶中没有内存时，就可以直接到 Central Cache 中相对应的哈希桶中取内存。

Central Cache 与 Thread Cache 不同之处有两点：

1. Central Cache 是所有线程共享的，而 Thread Cache 是线程独享的；
2. Central Cache 的哈希桶中挂载的是 Span，而 Thread Cache 的哈希桶中挂载的是切好的内存块；

其结构如下所示：

<div align="center">
<img src="./screenshot/centralcache.png" style="zoom:50%"/>
</div>

由于 Central Cache 是所有线程共享的，多个 Thread Cache 可能在同一时刻向 Central Cache 申请内存块，因此为了保证线程安全，需要加锁控制。此外，由于只有多个线程同时访问 Central Cache 的同一个桶时才会存在锁竞争，因此无需用锁来锁住所有哈希桶，只需锁住当前所访问的哈希桶即可。

当 Thread Cache 向 Central Cache 申请内存时，如果给的太少，那么 Thread Cache 在短时间用完了又会再来申请；但是如果给的太多，那么 Thread Cache 可能用不完而浪费大量的空间。为此，此处采用**慢反馈调节算法**，当 Thread Cache 向 Central Cache 申请内存时，如果申请的是较小的对象，那么可以多给一点，但如果申请的是较大的对象，就可以少给一点。

当 Thread Cache 中的某个自由链表太长时，会将自由链表中的对象归还给 Central Cache 中的 Span。但是需要注意的是，归还给 Central Cache 的这些对象不一定都属于同一个 Span 的，且 Central Cache 中的每个哈希桶中都可能不止一个 Span，因此归还时不仅需要知道该对象属于哪一个桶，还需要知道它属于这个桶中的哪一个 Span。为了建立页号和 Span 之间的映射，需要使用一种哈希表结构进行管理，一种方式是采用 C++ 中的 unordered_map，另一种方式是采用**基数树**数据结构。

### 3. PageCache

Page Cache 的结构与 Central Cache 一样，都是哈希桶的结构，并且 Page Cache 的每个哈希桶中都挂的是一个个的 Span，这些 Span 也是按照双向链表的结构连接起来的。

但是，Page Cache 的映射规则与 Central Cache 和 Thread Cache 不同，其采用的是**直接定址法**，比如 1 号桶挂的都是 1 页的 Span，2 号桶挂的都是 2 页的 Span，以此类推。

其次，Central Cache 每个桶中的 Span 都被切为了一个个对应大小的对象，以供 Thread Cache 申请。而 Page Cache 服务的是 Central Cache，当 Central Cache 中没有 Span 时，向 Page Cache 申请的是某一固定页数的 Span。而如果切分这个申请到的 Span 就应该由 Central Cache 自己来决定。

其结构如下图所示：

<div align="center">
<img src="./screenshot/pagecache.png" style="zoom:60%"/>
</div>

当每个线程的 Thread Cache 没有内存时都会向 Central Cache 申请，此时多个线程的 Thread Cache 如果访问的不是 Central Cache 的同一个桶，那么这些线程是可以同时进行访问的。这时 Central Cache 的多个桶就可能同时向 Page Cache 申请内存，所以 Page Cache 也是存在线程安全问题的，因此在访问 Page Cache 时也必须要加锁。

但是此处的 Page Cache 不能使用桶锁，因为当 Central Cache 向 Page Cache 申请内存时，Page Cache 可能会将其他桶中大页的 Span 切小后再给 Central Cache。此外，当 Central Cache 将某个 Span 归还给 Page Cache 时，Page Cache 也会尝试将该 Span 与其它桶当中的 Span 进行合并。

也就是说，在访问 Page Cache 时，可能同时需要访问多个哈希桶，如果使用桶锁则可能造成大量频繁的加锁和解锁，导致程序的效率底下。因此在访问 Page Cache 时没有使用桶锁，而是用一个大锁将整个 Page Cache 锁住。

如果 Central Cache 中有某个 Span 的 `useCnt_` 减到 0 了，那么 Central Cache 就需要将这个 Span 归还给 Page Cache 了。为了缓解内存碎片问题，Page Cache 还需要尝试将还回来的 Span 与其它空闲的 Span 进行合并。

## 网络通信模块

网络通信模块采用的是 muduo 网络库，本项目通过使用 C++11 简化 muduo 网络库，同时去除了 Boost 库的依赖以及一些冗余的组件，提取出 muduo 库中的核心思想，即 One Loop Per Thread。

### 1. Reactor

该网络库采用的是 Reactor 事件处理模式。在《Linux高性能服务器编程》中，对于 Reactor 模型的描述如下：**主线程（即 I/O 处理单元）只负责监听文件描述符上是否有事件发生，有的话就立即将该事件通知工作线程（即逻辑单元）。此外，主线程不做任何其他实质性的工作。读写数据、接受新的连接，以及处理客户请求均在工作线程中完成**。Reactor 模式的时序图如下：

<div align="center">
<img src="./screenshot/reactor-sequence.png" style="zoom:60%"/>
</div>

而 muduo 网络库的时序图则如下图所示：

<div align="center">
<img src="./screenshot/muduo-sequence.png" style="zoom:60%">
</div>

其次，在《Linux高性能服务器编程》一书中还提到了**半同步/半异步**的并发模式，注意，此处的“异步”与 I/O 模型中的异步并不相同，I/O 模型中的“同步”和“异步”的区分是内核应用程序通知的是何种事件（就绪事件还是完成事件），以及由谁来完成 I/O 读写（是应用程序还是内核）。而在并发模式中，“同步”指的是完全按照代码序列的顺序执行，“异步”则指的是程序的执行需要由系统事件来驱动，比如常见的系统终端、信号等。

而 muduo 库所采用的便是高效的半同步/半异步模式，其结构如下图所示：

<div align="center">
<img src="./screenshot/half-sync-asyn.png" style="zoom:60%"/>
</div>

上图中，主线程只管理监听 socket，连接 socket 由工作线程来管理。当有新的连接到来时，主线程就接受并将新返回的连接 socket 派发给某个工作线程，此后在该 socket 上的任何 I/O 操作都由被选中的工作线程来处理，直到客户关闭连接。主线程向工作线程派发 socket 的最简单的方式，是往它和工作线程之间的管道写数据。工作线程检测到管道上有数据可读时，就分析是否是一个新的客户连接请求到来。如果是，则把新 socket 上的读写事件注册到自己的 epoll 内核事件表中。上图中的每个线程都维持自己的事件循环，它们各自独立的监听不同的事件。因此，在这种高效的半同步/半异步模式中，每个线程都工作在异步模式，所以它并非严格意义上的半同步/半异步模式。

通常情况下，Reactor 模式的实现有如三几种方式：

- Single Reactor - Single Thread
- Single Reactor - Multi Threads
- Multi Reactors - Multi Threads

对于 “Single Reactor - Single Thread” 模型而言，其通常只有一个 epoll 对象，所有的接收客户端连接、客户端读取、客户端写入操作都包含在一个线程内，如下图所示：

<div align="center">
<img src="./screenshot/reactor-thread.png" style="zoom:50%"/>
</div>

但在目前的单线程 Reactor 模式中，不仅 I/O 操作在该 Reactor 线程上，连非 I/O 的业务操作也在该线程上进行处理了，这可能会大大延迟 I/O 请求的响应。为了提高服务器的性能，我们需要将非 I/O 的业务逻辑操作从 Reactor 线程中移动到工作线程中进行处理。

为此，可以通过使用线程池模型的方法来改进，即 “Single Reactor - Multi Threads” 模型，其结构如下图所示。将读写的业务逻辑交给具体的线程池来实现，这样可以显示 reactor 线程对 IO 的响应，以此提升系统性能。

<div align="center">
<img src="./screenshot/reactor-threads.png" style="zoom:50%"/>
</div>

尽管现在已经将所有的非 I/O 操作交给了线程池来处理，但是所有的 I/O 操作依然由 Reactor 单线程执行，在高负载、高并发或大数据量的应用场景，依然较容易成为瓶颈。

为了继续提升服务器的性能，进而改造出了如下图所示的 “Multi Reactors - Multi Threads” 模型：

<div align="center">
<img src="./screenshot/reactors-threads.png" style="zoom:50%"/>
</div>

在这种模型中，主要分为两个部分：mainReactor、subReactors。 mainReactor 主要负责接收客户端的连接，然后将建立的客户端连接通过负载均衡的方式分发给 subReactors，subReactors 则负责具体的每个连接的读写，而对于非 IO 的操作，依然交给工作线程池去做，对逻辑进行解耦。

而在 muduo 网络库中，便采用的是此种模型，每一个 Reactor 都是一个 EventLoop 对象，而每一个 EventLoop 则和一个线程唯一绑定，这也就是 One Loop Per Thread 的含义所在。其中，MainLoop 只负责新连接的建立，连接建立成功后则将其打包为 TcpConnection 对象分发给 SubLoop，在 muduo 网络库中，采用的是 “轮询算法” 来选择处理客户端连接的 SubLoop。之后，这个已建立连接的任何操作都交付给该 SubLoop 来处理。

通常在服务器模型中，我们可以使用 “任务队列” 的方式向 SubLoop 派发任务，即 MainLoop 将需要执行的任务放到任务队列中，而 SubLoop 则从任务队列中取出任务并执行，当任务队列中没有任务时，SubLoop 则进行休眠直到任务队列中有任务出现。但是在 muduo 网络库却中并未采用这一方式，而是采用了另一个更加高效的方式，以便让 MainLoop 唤醒 SubLoop 处理任务。

在上述的 “半同步/半异步” 模式中，我们提到了，主线程向工作线程派发 socket 最简单的方式，就是往它和工作线程之间的管道写数据。为此，我们可以在 MainLoop 和 SubLoop 之间建立管道来进行通信，当有任务需要执行时，MainLoop 通过管道将数据发送给 SubLoop，SubLoop 则通过 epoll 模型监听到了管道上所发生的可读（EPOLLIN）事件，然后调用相应的读事件回调函数来处理任务。

但是在 muduo 库中，则采用了更为高效的 `eventfd()` 接口，它通过创建一个文件描述符用于事件通知，自 Linux 2.6.22 以后开始支持。

> eventfd 在信号通知的场景下，相对比 pipe 有非常大的资源和性能优势，它们的对比如下：
> 1. 首先在于它们所打开的文件数量的差异，由于 pipe 是半双工的传统 IPC 实现方式，所以两个线程通信需要两个 pipe 文件描述符，而用 eventfd 则只需要打开一个文件描述符。总所周知，文件描述符是系统中非常宝贵的资源，Linux 的默认值只有 1024 个，其次，pipe 只能在两个进程/线程间使用，面向连接，使用之前就需要创建好两个 pipe ,而 eventfd 是广播式的通知，可以多对多。
> 2. 另一方面则是内存使用的差别，eventfd 是一个计数器，内核维护的成本非常低，大概是自旋锁+唤醒队列的大小，8 个字节的传输成本也微乎其微，而 pipe 则完全不同，一来一回的数据在用户空间和内核空间有多达 4 次的复制，而且最糟糕的是，内核要为每个 pipe 分配最少 4K 的虚拟内存页，哪怕传送的数据长度为 0。

### 2. I/O multiplexing

在 Linux 系统下，常见的 I/O 复用机制有三种：select、poll 和 epoll。

其中，select 模型的缺点如下：

1. 单个进程能够监视的文件描述符的数量存在最大限制，通常是 1024，当然可以更改数量，但由于 select 采用轮询的方式扫描文件描述符，文件描述符数量越多，性能越差；
2. 内核和用户空间的内存拷贝问题，select 需要复制大量的句柄数据结构，会产生巨大的开销；
3. select 返回的是含有整个句柄的数组，应用程序需要遍历整个数组才能发现哪些句柄发生了事件；
4. select 的触发方式是水平触发，应用程序如果没有对一个已经就绪的文件描述符进行相应的 I/O 操作，那么之后每次 select 调用还是会将这些文件描述符通知进程；

相比于 select 模型，poll 则使用链表来保存文件描述符，因此没有了监视文件数量的限制，但其他三个缺点依然存在。

而 epoll 的实现机制与 select/poll 机制完全不同，它们的缺点在 epoll 模型上不复存在。其高效的原因有以下两点：

1. 它通过使用**红黑树**这种数据结构来存储 epoll 所监听的套接字。当添加或者删除一个套接字时（epoll_ctl），都是在红黑树上进行处理，由于红黑树本身插入和删除性能比较好，时间复杂度为 O(logN)，因此其效率要高于 select/poll。
2. 当把事件添加进来的时候时候会完成关键的一步，那就是该事件会与相应的设备（网卡）驱动程序建立回调关系，当相应的事件发生后，就会调用这个回调函数。这个回调函数其实就是把该事件添加到 `rdllist` 这个双向链表中。那么当我们调用 epoll_wait 时，epoll_wait 只需要检查 rdlist 双向链表中是否有存在注册的事件，效率非常可观。

epoll 对文件描述符的操作有两种模式：LT（Level Trigger，电平触发）和 ET（Edge Trigger，边沿触发）模式。其中，LT 模式是默认的工作模式，这种模式下 epoll 相当于一个效率较高的 poll。当往 epoll 内核事件表中注册一个文件描述符上的 EPOLLOUT 事件时，epoll 将以 ET 模式来操作该文件描述符。ET 模式是 epoll 的高效工作模式。

对于采用 LT 工作模式的文件描述符，当 epoll_wait 检测到其上有事件发生并将此事件通知应用程序后，应用程序可以不立即处理该事件。这样，当应用程序下一次调用 epoll_wait 时，epoll_wait 还会再次向应用程序通告此事件，直到该事件被处理。而对于采用 ET 工作模式的文件描述符，当 epoll_wait 检测到其上有事件发生并将此事件通知应用程序后，应用程序必须立即处理该事件，因为后续的 epoll_wait 调用将不再向应用程序通知这一事件。可见，ET 模式在很大程度上降低了同一个 epoll 事件被重复触发的此时，因此效率要比 LT 模式高。

在 muduo 网络库中，则采用了 LT 工作模式，其原因如下：

1. 不会丢失数据或者消息，应用没有读取完数据，内核是会不断上报的；
2. 每次读数据只需要一次系统调用；照顾了多个连接的公平性，不会因为某个连接上的数据量过大而影响其他连接处理消息；

## 日志模块

## 分布式
