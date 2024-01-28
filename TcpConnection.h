#if !defined(MUDUO_TCPCONNECTION_H)
#define MUDUO_TCPCONNECTION_H

#include <muduo/base/allocator/Allocatable.h>
#include <muduo/Buffer.h>
#include <muduo/InetAddr.h>
#include <muduo/TcpServer.h>  // for declare friend
#include <muduo/Callbacks.h>
#include <functional>
#include <memory>
#include <string>
#include <any>

namespace muduo {
    
class EventLoop;        // forward declaration
class Channel;
class Socket;

class TcpConnection : public std::enable_shared_from_this<TcpConnection>
#ifdef MUDUO_USE_MEMPOOL
                    , public base::detail::Allocatable {
#else
                    {
#endif
    friend void TcpServer::HandleNewConnection(int connfd, const InetAddr& remote_addr);
    friend void TcpServer::RemoveConnectionInLoop(const TcpConnectionPtr& conn);
    friend TcpServer::~TcpServer() noexcept;

    /* non-copyable and non-moveable*/
    TcpConnection(const TcpConnection&) = delete;
    TcpConnection& operator=(const TcpConnection&) = delete;

    using CloseCallback_t = std::function<void(const TcpConnectionPtr& conn)>;
    enum State { connecting, connected, disconnecting, disconnected };

public:
    TcpConnection(EventLoop* owner, const std::string& name, int sockfd, const InetAddr& local_addr, const InetAddr& remote_addr);
    ~TcpConnection() noexcept;

    EventLoop* GetEventLoop() { return loop_; }
    const InetAddr& GetLocalAddr() const { return localAddr_; } 
    const InetAddr& GetRemoteAddr() const { return remoteAddr_; } 
    const std::string& GetName() const { return name_; }

    void SetTcpNoDelay(bool on);

    void SetContext(const std::any& ctx) { context_ = ctx; }
    const std::any& GetContext() const { return context_; }
    /// @brief Return mutable context
    std::any& GetContext() { return context_; }
    bool IsConnected() { return state_ == connected; }
    void SetConnectionCallback(const ConnectionCallback_t& cb)
    { connectionCb_ = cb; }
    void SetOnMessageCallback(const MessageCallback_t& cb)
    { onMessageCb_ = cb; }
    void SetWriteCompleteCallback(const WriteCompleteCallback_t& cb)
    { writeCompleteCb_ = cb; }
    void SetHighWaterMarkCallback(size_t mark, const HighWaterMarkCallback_t& cb)
    { highWaterMark_ = mark; highWaterCb_ = cb; }


    /// Thread-safe, can call cross-thread
    void Shutdown();
    
    /// Thread-safe, can call cross-thread
    void Send(const std::string& s)
    { Send(s.c_str(), s.size()); }
    
    /// Thread-safe, can call cross-thread
    void Send(const void* buf, size_t len)
    { Send(static_cast<const char*>(buf), len); }

    /// Thread-safe, can call cross-thread
    void Send(const char* buf, size_t len);

private:
    /// @note Only used by muduo::TcpServer
    void SetOnCloseCallback(const CloseCallback_t& cb)
    { onCloseCb_ = cb; }
    void StepIntoEstablished();
    void StepIntoDestroyed();
    void ShutdownInLoop();

    void SendInLoop(const char* buf, size_t len);

    /* Reactor-handlers */
    void HandleClose();
    void HandleError();
    void HandleRead(const ReceiveTimePoint_t&);
    void HandleWrite();


private:
    EventLoop* loop_;
    std::string name_;
    InetAddr localAddr_;
    InetAddr remoteAddr_;
    std::unique_ptr<Socket, std::function<void(Socket*)>> socket_;
    std::unique_ptr<Channel, std::function<void(Channel*)>> chan_;
    std::atomic<State> state_ {connecting};
    std::any context_ {};  // C++ 17
    ConnectionCallback_t connectionCb_ {nullptr};
    MessageCallback_t onMessageCb_ {nullptr};
    CloseCallback_t onCloseCb_ {nullptr};   // invoke TcpServer::RemoveConnection of Associated TCP-Server
    WriteCompleteCallback_t writeCompleteCb_ {nullptr};
    HighWaterMarkCallback_t highWaterCb_ {nullptr};
    size_t highWaterMark_ {0};

    Buffer inputBuffer_;
    Buffer outputBuffer_;
};

} // namespace muduo 
#endif // MUDUO_TCPCONNECTION_H
