#if !defined(MUDUO_TCPSERVER_H)
#define MUDUO_TCPSERVER_H
#include <Callbacks.h>
#include <unordered_map>
#include <atomic>
#include <memory>
namespace muduo {

class EventLoop;        // forward declaration
class Acceptor;         // forward declaration
class TcpConnection;    // forward declaration
class EventLoopThreadPool;  // forward declaration

/// @brief A non-copyable TCP-Server
/// single-Reactor mode, Acceptor and IO-handler run in same thread
class TcpServer {
    friend TcpConnection;
    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

public:
    explicit TcpServer(EventLoop* loop, const InetAddr& addr, const std::string& name);
    ~TcpServer() noexcept;  // force out-line dtor, for std::unique_ptr members.
    std::string GetIp() const;
    std::string GetIpPort() const;
    void ListenAndServe();

    void SetIoThreadNum(int n);
    void SetIothreadInitCallback(const IoThreadInitCallback_t& cb);

    void SetConnectionCallback(const ConnectionCallback_t& cb)
    { connectionCb_ = cb; }
    void SetOnMessageCallback(const MessageCallback_t& cb)
    { messageCb_ = cb; }
    void SetOnWriteCompleteCallback(const WriteCompleteCallback_t& cb)
    { writeCompleteCb_ = cb; }

private:
    void HandleNewConnection(int connfd, const InetAddr& remote_addr);
    void RemoveConnection(const TcpConnectionPtr& conn);
    void RemoveConnectionInLoop(const TcpConnectionPtr& conn);

private:
    EventLoop* loop_;
    std::string name_;
    std::unique_ptr<InetAddr> addr_;
    std::unique_ptr<Acceptor> acceptor_;
    std::unique_ptr<EventLoopThreadPool> ioThreadPool_;
    std::atomic_bool serving_ {false};
    
    using ConnectionsMap = std::unordered_map<std::string, TcpConnectionPtr>;
    ConnectionsMap conns_;

    /* Callbacks for custom logic */
    ConnectionCallback_t connectionCb_ {nullptr};
    MessageCallback_t messageCb_ {nullptr};
    WriteCompleteCallback_t writeCompleteCb_ {nullptr};
    /* allways in loop-thread */
    uint64_t nextConnID_ {0};
};

} // namespace muduo 

#endif // MUDUO_TCPSERVER_H
