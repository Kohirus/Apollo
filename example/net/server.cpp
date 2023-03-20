#include "tcpserver.hpp"
#include "eventloop.hpp"

int main() {
    std::shared_ptr<apollo::EventLoop> loop(new apollo::EventLoop);
    apollo::TcpServer svr(loop, "127.0.0.1", 7777);
    loop->process();
    return 0;
}