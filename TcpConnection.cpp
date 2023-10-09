#include <TcpConnection.h>
#include <EventLoop.h>
#include <Socket.h>
using namespace muduo;

TcpConnection::TcpConnection(EventLoop* owner, const std::string& name, int sockfd, const InetAddr& local_addr, const InetAddr& remote_addr)
    : owner_(owner)
    , name_(name)
    , socket_(std::make_unique<Socket>(sockfd))
    , localAddr_(local_addr)
    , remoteAddr_(remote_addr)
    { }

TcpConnection::~TcpConnection() noexcept {
    // TODO: ...
}

