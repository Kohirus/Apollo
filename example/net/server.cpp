#include "tcpserver.hpp"

int main() {
    apollo::TcpServer svr("127.0.0.1", 7777);
    svr.doAccept();
    return 0;
}