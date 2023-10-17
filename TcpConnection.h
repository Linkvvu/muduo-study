#if !defined(MUDUO_TCPCONNECTION_H)
#define MUDUO_TCPCONNECTION_H
#include <InetAddr.h>
#include <TcpServer.h>  // for declare friend
#include <Callbacks.h>
#include <memory>
#include <string>
#include <any>

namespace muduo {
    
class EventLoop;        // forward declaration
class Channel;          // forward declaration
class Socket;           // forward declaration

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
    friend void TcpServer::HandleNewConnection(int connfd, const InetAddr& remote_addr);
    friend void TcpServer::RemoveConnection(const TcpConnectionPtr& conn);
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

    void SetContext(const std::any& ctx) { context_ = ctx; }
    const std::any& GetContext() const { return context_; }
    /// @brief Return mutable context
    std::any& GetContext() { return context_; }
    bool IsConnected() { return state_ == connected; }
    void SetConnectionCallback(const ConnectionCallback_t& cb)
    { connectionCb_ = cb; }
    void SetOnMessageCallback(const MessageCallback_t& cb)
    { onMessageCb_ = cb; }

    /// Thread-safe, can call cross-thread
    void Shutdown();
    
private:
    /// @note Only used by muduo::TcpServer
    void SetOnCloseCallback(const CloseCallback_t& cb)
    { onCloseCb_ = cb; }
    void StepIntoEstablished();
    void StepIntoDestroyed();
    void ShutdownInLoop();

    /* Reactor-handlers */
    void HandleClose();
    void HandleError();
    void HandleRead(const ReceiveTimePoint_t&);
    void HandleWrite();


private:
    EventLoop* loop_;
    std::string name_;
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> chan_;
    InetAddr localAddr_;
    InetAddr remoteAddr_;
    std::atomic<State> state_ {connecting};
    std::any context_ {nullptr};  // C++ 17
    ConnectionCallback_t connectionCb_ {nullptr};
    MessageCallback_t onMessageCb_ {nullptr};
    CloseCallback_t onCloseCb_ {nullptr};   // invoke TcpServer::RemoveConnection of Associated TCP-Server
};

} // namespace muduo 
#endif // MUDUO_TCPCONNECTION_H
