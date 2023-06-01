#include <muduo/net/socket.h>
#include <muduo/net/channel.h>
#include <muduo/base/logging.h>
#include <muduo/net/eventLoop.h>
#include <muduo/net/socketsOPs.h>
#include <muduo/base/timeStamp.h>
#include <muduo/net/tcpConnection.h>
using namespace muduo;
using namespace muduo::net;

tcp_connection::tcp_connection(event_loop* const loop, const string& name, const int sockfd, const inet_address& localAddr, const inet_address& peerAddr)
    : loop_(loop)
    , name_(name)
    , connSocket_(std::make_unique<socket>(sockfd))
    , connChannel_(std::make_unique<channel>(loop_, sockfd))
    , stage_(stage::connecting) // 当前处于正在连接阶段
    , localAddr_(localAddr)
    , peerAddr_(peerAddr)
    , onMsgCb_(nullptr)
    , onConnCb_(nullptr)
    , writeCompleteCb_(nullptr)
    , input_buffer_()
    , output_buffer_() {
        connChannel_->setReadCallback(std::bind(&tcp_connection::handle_read, this, std::placeholders::_1));
        connChannel_->setErrorCallback(std::bind(&tcp_connection::handle_error, this));
        connChannel_->setWriteCallback(std::bind(&tcp_connection::handle_write, this));
        connChannel_->setCloseCallback(std::bind(&tcp_connection::handle_close, this));
    }

tcp_connection::~tcp_connection() {
    LOG_DEBUG << "tcp_connection::dtor[" <<  name_ << "] at " << this << " fd=" << connSocket_->fd();
}

void tcp_connection::step_into_established() {
    loop_->assert_loop_in_hold_thread();

    assert(stage_ == stage::connecting);
    stage_ = stage::connected;      // 步入连接建立阶段
    connChannel_->tie(shared_from_this());
    connChannel_->enableReading();  // 将当前tcp连接所对应的通道加入至event-loop(poller)中
    onConnCb_(shared_from_this());  // 连接成功, 调用应用层注册的回调函数
}

void tcp_connection::handle_read(TimeStamp recv_time) {
    loop_->assert_loop_in_hold_thread();
    int save_error = 0;
    auto n = input_buffer_.read_fd(connSocket_->fd(), &save_error);
    if (n > 0) {
        if (onMsgCb_)
            onMsgCb_(shared_from_this(), &input_buffer_, recv_time);
    } else if (n == 0) {
        handle_close();    
    } else {
        LOG_ERROR << "connection [" << name() << "]  occur a error: " << std::strerror(save_error);
        handle_error();
    }
}

void tcp_connection::handle_close() {
    loop_->assert_loop_in_hold_thread();
    assert(stage_ == stage::connected || stage_ == stage::disconnecting);
    set_stage(stage::disconnected);
    connChannel_->disableAllEvents();   // prevent poll trigger POLLOUT again

    tcp_connection_ptr guard = shared_from_this();
    onCloseCb_(guard);                  // invoke tcp_server::remove_connection
}

/// issue: when tcp-server destructing, tcp-connection stage is stage::disconnecting, what will happen ?
void tcp_connection::connection_destroy() {
    loop_->assert_loop_in_hold_thread();
    // optimize: use CAS - compare and swap
    if (stage_ == stage::connected) {
        set_stage(stage::disconnected);
        connChannel_->disableAllEvents();
    }
    onConnCb_(shared_from_this());      // 即将断开连接, 调用应用层注册的回调函数
    connChannel_->remove();
}

void tcp_connection::handle_error() {
    loop_->assert_loop_in_hold_thread();
    int err = sockets::get_socket_error(connChannel_->fd());
    LOG_ERROR << "tcp_connection::handle_error [" << name_ << "] - SO_ERROR = " << err << " " << std::strerror(err);
}

void tcp_connection::send(const void* data, std::size_t len) {
    if (stage_ == stage::connected) {
        if (loop_->is_in_eventLoop_thread()) {
            send_in_loop(data, len);
        } else {
            string tmp(static_cast<const char*>(data), len);
            loop_->enqueue_eventLoop([this, s = std::move(tmp)]() {
                this->send_in_loop(s.data(), s.size());
            });
        }
    }
}

void tcp_connection::send_in_loop(const void* data, std::size_t len) {
    loop_->assert_loop_in_hold_thread();
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;
    if (stage_ == stage::disconnected) {
        LOG_WARN << "disconnected, give up writing";
        return;
    }

    // if no thing in output queue, try writing directly
    if (!connChannel_->isWriting() && output_buffer_.readable_bytes() == 0) {
        nwrote = sockets::write(connChannel_->fd(), data, len);
        if (nwrote >= 0) {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCb_) {
                loop_->enqueue_eventLoop(std::bind(writeCompleteCb_, shared_from_this()));
            }
        } else {    // nwrote == -1 - occur a error
            nwrote = 0;
            if (errno != EAGAIN || errno != EWOULDBLOCK) {  // has other error?
                LOG_SYSERR << "connection [" << name() << "]  occur a error";
                if (errno == EPIPE || errno == ECONNRESET)
                    faultError = true;
            }
        }
    }

    assert(remaining <= len);
    
    if (!faultError && remaining > 0) {
        auto old_len = output_buffer_.readable_bytes();
        if (old_len+remaining >= highWaterMark_ && old_len < highWaterMark_ && highWaterMarkCb_) {
            loop_->enqueue_eventLoop([self = shared_from_this(), old_len, len]() {
                self->highWaterMarkCb_(self, old_len + len);
            });
        }
        output_buffer_.append(static_cast<const char*>(data) + nwrote, remaining);  // add remaining data to the output-buffer
        if (!connChannel_->isWriting()) {
            connChannel_->enableWriting();
        }
    }
}

void tcp_connection::handle_write() {
    loop_->assert_loop_in_hold_thread();
    assert(stage_ == stage::connected || stage_ == stage::disconnecting);
    if (connChannel_->isWriting()) {
        ssize_t n = sockets::write(connChannel_->fd(), output_buffer_.peek(), output_buffer_.readable_bytes());
        if (n > 0) {
            output_buffer_.retrieve(static_cast<std::size_t>(n));
            if (output_buffer_.readable_bytes() == 0) {
                if (writeCompleteCb_)
                    writeCompleteCb_(shared_from_this());
                if (stage_ == stage::disconnecting) {
                    shutdown_in_loop();
                }
            }
        } else {    // n == 0: kernel buffer full
            LOG_SYSERR << "connection [" << name() << "]  occur a error";
        }
    } else {
        LOG_TRACE << "connection fd = " << connChannel_->fd() << " is down, no more writing";
    }
}

void tcp_connection::shutdown() {
    // optimize: use CAS
    if (stage_ == stage::connected) {
        set_stage(stage::disconnecting);
        loop_->run_in_eventLoop_thread([this]() { this->shutdown_in_loop(); });
    }
}

void tcp_connection::shutdown_in_loop() {
    loop_->assert_loop_in_hold_thread();
    if (!connChannel_->isWriting()) {
        connSocket_->shutdownWrite();
    }
}

void tcp_connection::set_tcp_noDelay(bool on) {
    connSocket_->set_tcp_noDelay(on);
}