#if !defined(MUDUO_TCPCONNECTION_H)
#define MUDUO_TCPCONNECTION_H
#include <InetAddr.h>
#include <Callbacks.h>
#include <memory>
#include <string>
#include <any>

namespace muduo {
    
class EventLoop;        // forward declaration
class Channel;          // forward declaration
class Socket;           // forward declaration

class TcpConnection {
    TcpConnection(const TcpConnection&) = delete;
    TcpConnection& operator=(const TcpConnection&) = delete;
    
public:
    TcpConnection(EventLoop* owner, const std::string& name, int sockfd, const InetAddr& local_addr, const InetAddr& remote_addr);
    ~TcpConnection() noexcept;

    EventLoop* GetEventLoop() { return owner_; }
    const InetAddr& GetLocalAddr() const { return localAddr_; } 
    const InetAddr& GetRemoteAddr() const { return remoteAddr_; } 
    const std::string& GetName() const { return name_; }

    void SetContext(const std::any& ctx) { context_ = ctx; }
    const std::any& GetContext() const { return context_; }
    /// @brief Return mutable context
    std::any& GetContext() { return context_; }

    void SetConnectionCallback(const ConnectionCallback_t& cb)
    { connectionCb_ = cb; }
    void SetOnMessageCallback(const MessageCallback_t& cb)
    { onMessageCb_ = cb; }

private:
    /// @note Only used by muduo::TcpServer
    void SetOnCloseCallback(const CloseCallback_t& cb)
    { onCloseCb_ = cb; }


private:
    EventLoop* owner_;
    std::string name_;
    std::unique_ptr<Socket> socket_;
    InetAddr localAddr_;
    InetAddr remoteAddr_;
    std::any context_ {nullptr};  // C++ 17
    ConnectionCallback_t connectionCb_ {nullptr};
    CloseCallback_t onCloseCb_ {nullptr};
    MessageCallback_t onMessageCb_ {nullptr};
};

} // namespace muduo 
#endif // MUDUO_TCPCONNECTION_H
