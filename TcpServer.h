#if !defined(MUDUO_TCPSERVER_H)
#define MUDUO_TCPSERVER_H

#include <muduo/base/allocator/Allocatable.h>
#include <muduo/base/allocator/sgi_stl_alloc.h>
#include <muduo/InetAddr.h>
#include <muduo/Callbacks.h>
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
#ifdef MUDUO_USE_MEMPOOL
class TcpServer : public base::detail::Allocatable {
    using ConnectionsMap = std::unordered_map<std::string, TcpConnectionPtr, 
                                            std::hash<std::string>, std::equal_to<std::string>,
                                            base::allocator<std::pair<const std::string, TcpConnectionPtr>>>;
#else
class TcpServer {
    using ConnectionsMap = std::unordered_map<std::string, TcpConnectionPtr>;
#endif
    friend TcpConnection;
    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

public:
    static std::unique_ptr<TcpServer> Create(EventLoop* loop, const InetAddr& addr, const std::string& name);

    explicit TcpServer(EventLoop* loop, const InetAddr& addr, const std::string& name);
    ~TcpServer() noexcept;  // force out-line dtor, for std::unique_ptr members.
    std::string GetIp() const;
    std::string GetIpPort() const;

    /// start io-threads and listening 
    /// @note Enables cross-thread invocation and utilizes Compare-and-Swap (CAS) internally,
    /// allowing for multiple invocations.
    void ListenAndServe();

    /// must call before TcpServer::ListenAndServe
    void SetIoThreadNum(int n);

    /// must call before TcpServer::ListenAndServe
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
    InetAddr addr_;
    std::unique_ptr<Acceptor> acceptor_;
    std::unique_ptr<EventLoopThreadPool> ioThreadPool_;
    ConnectionsMap conns_;
    std::atomic_bool serving_ {false};

    /* Callbacks for custom logic */
    ConnectionCallback_t connectionCb_ {nullptr};
    MessageCallback_t messageCb_ {nullptr};
    WriteCompleteCallback_t writeCompleteCb_ {nullptr};
    /* always in loop-thread */
    uint64_t nextConnID_ {0};
};

} // namespace muduo 

#endif // MUDUO_TCPSERVER_H
