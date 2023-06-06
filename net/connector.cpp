#include <muduo/net/channel.h>
#include <muduo/base/logging.h>
#include <muduo/net/connector.h>
#include <muduo/net/eventLoop.h>
#include <muduo/net/socketsOPs.h>
#include <muduo/net/timerQueue.h>
#include <muduo/net/timerId.h>

using namespace muduo;
using namespace muduo::net;

connector::connector(event_loop* const loop, const inet_address& raddr, const newConnCallback_t& func)
    : loop_(loop)
    , peerAddr_(raddr)
    , helperChannel_(nullptr)
    , newConnCb_(func)
    , delayTime_(KInitRetryDelayMs)
    , isConnect_(false)
    , stage_(stage::disconnected) {

    }

connector::~connector() {
    assert(!helperChannel_.operator bool());
}

void connector::start_connect() {
    isConnect_ = true;
    loop_->run_in_eventLoop_thread([this]() {
        this->start_connect_in_loop();
    });
}

void connector::start_connect_in_loop() {
    loop_->assert_loop_in_hold_thread();
    assert(stage_ == stage::disconnected);  // maybe should use 'if' judge
    if (isConnect_) {
        connect();
    } else {
        LOG_DEBUG << "no not connect in connector::start_connect_in_loop";
    }
}

void connector::connect() {
    loop_->assert_loop_in_hold_thread();
    auto sockfd = sockets::createNon_blockingOrDie();   // create a non-block socket
    auto ret = sockets::connect(sockfd, &peerAddr_.get_sock_inetAddr());
    int saved_error = (ret == 0 ? 0 : errno);
    switch (saved_error)
    {
    case 0:
    case EINPROGRESS:   // non-block正在进行异步连接
    case EINTR:         // 连接被信号中断，将异步进行连接
    case EISCONN:       // 该socket已经被连接
        connecting(sockfd);
        break;
    
    case EADDRINUSE:    // 尝试使用已经在使用的地址建立连接
    case ECONNRESET:    // server 全连接队列已满
    case EADDRNOTAVAIL: // 指定的地址在本机不可用
    case ECONNREFUSED:  // 没有到该网络的路由
        // retry();
        break;

    case EACCES:        // Write access to the specified socket is denied.
    case EALREADY:      // The specified address is not a valid address for the address family of the specified socket.
    case EBADF:         // The socket argument is not a valid file descriptor.
    case ENOTSOCK:      // The socket argument does not refer to a socket.
        LOG_SYSERR << "connect error in connector::start_connect_in_loop " << saved_error;
        sockets::close(sockfd);
        break;
    default:
        LOG_SYSERR << "unexpected error in connector::start_connect_in_loop " << saved_error;
        sockets::close(sockfd);
        break;
    }
}

void connector::connecting(int sockfd) {
    assert(!helperChannel_.operator bool());
    stage_ = stage::connecting;
    helperChannel_.reset(new channel(loop_, sockfd, "connector helper"));   // 关联 sockfd 和 channel
    helperChannel_->setWriteCallback([this]() { this->handle_write(); });
    helperChannel_->setErrorCallback([this]() { this->handle_error(); });
    
    // helperChannel_->tie(shared_from_this()); is not working,
    // as helperChannel_ is not managed by shared_ptr
    // 因为 connector类(对象) 不是通过shared_ptr所管理的
    // 又因为 helperChannel_ 是connector的类成员，故其也不是通过shared_ptr所管理的
    helperChannel_->enableWriting();    // 关注write event：若write被触发，则表示connect(3)函数工作已经完成、但并不一定连接成功
}

void connector::handle_write() {
    loop_->assert_loop_in_hold_thread();
    if (stage_ == stage::connecting) {  // usually case
        int sockfd = remove_and_resetChannel(); // prevent busy loop; 本次connect(3)已完成，无需再关注了
        int saved_error = sockets::get_socket_error(sockfd);    // Check whether connect(3) succeeded
        if (saved_error) {  // !0 -> occurred a error
            LOG_ERROR << "connect occurred a error[" << saved_error << "]: " << std::strerror(saved_error);
            retry(sockfd);  // reconnect
        } else if (sockets::is_self_connect(sockfd)) {    // Check whether is self connection
            LOG_WARN << "Connector::handleWrite - Self connect";
            retry(sockfd);  // reconnect
        } else {    // connect success
            if (isConnect_) {
                stage_ = stage::connected;
                newConnCb_(sockfd);
            } else {
                sockets::close(sockfd);
                stage_ = stage::disconnected;
            }
        }
    } else {
        assert(stage_ == stage::disconnected);  // 可能调用完connector::start_connect后立刻又调用了connector::stop
    }
}

//  use back-off strategy
void connector::retry(int sockfd) {
    sockets::close(sockfd);
    stage_ = stage::disconnected;
    if (isConnect_) {
        LOG_INFO << "retry attempt to connectint to " << peerAddr_.ipAndPort() << " in " << delayTime_ << " milliseconds."; 
        loop_->run_after(delayTime_ / 1000.0, [this]() { this->start_connect_in_loop(); });
        delayTime_ = std::min(KMaxDelayTime_ms, delayTime_*2);  // back-off strategy
    } else {
        LOG_DEBUG << "no not connect in connector::retry";
    }
}

void connector::handle_error() {
    loop_->assert_loop_in_hold_thread();

    assert(stage_ == stage::connecting);
    int sockfd = remove_and_resetChannel();
    int saved_error = sockets::get_socket_error(sockfd);
    sockets::close(sockfd);
    LOG_ERROR << "SO_ERROR = " << saved_error << " " << std::strerror(saved_error) << " - connector::handle_error";
    stage_ = stage::disconnected;
}

int connector::remove_and_resetChannel() { // called by connector::handle_write OR connector::handle_error OR connector::stop_connect_in_loop
    // loop_->assert_loop_in_hold_thread(); // it 100% be called by connector::handle_write OR connector::handle_error OR connector::stop_connect_in_loop
    assert(loop_->is_in_eventLoop_thread());
    int sockfd = helperChannel_->fd();
    helperChannel_->disableAllEvents();
    helperChannel_->remove();
    // helperChannel_.reset();  // WARN: Can't reset helperChannel_ here, because we are inside channel::handle_event
    loop_->enqueue_eventLoop([this]() { this->helperChannel_.reset(); });
    return sockfd;
}

void connector::stop_connect() {
    isConnect_ = false;
    loop_->run_in_eventLoop_thread([this]() {
        this->stop_connect_in_loop();
    });
}

void connector::stop_connect_in_loop() {
    loop_->assert_loop_in_hold_thread();
    stage expected = stage::connecting;
    bool is_swap = stage_.compare_exchange_strong(expected, stage::disconnected);
    if (is_swap) {
        int sockfd = remove_and_resetChannel();
        sockets::close(sockfd);
    }
}

void connector::restart_connect() {
    loop_->assert_loop_in_hold_thread();
    stage_ = stage::disconnected;
    delayTime_ = KInitRetryDelayMs;
    isConnect_ = true;
    start_connect_in_loop();
}