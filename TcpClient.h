#if !defined(MUDUO_TCP_CLIENT_H)
#define MUDUO_TCP_CLIENT_H


#include <muduo/EventLoop.h>
#include <muduo/InetAddr.h>

namespace muduo {

class Connector;        // forward declaration
class TcpConnection;    // forward declaration
class TcpClient;        // forward declaration
using ConnectorPtr = std::shared_ptr<Connector>;
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using TcpClientPtr = std::shared_ptr<TcpClient>;

/// @brief Factory method, create a tcp-client instance
extern TcpClientPtr CreateTcpClient(EventLoop* loop, const InetAddr& server_addr, std::string name);

/// Must be managed by @c std::shared_ptr
class TcpClient : public std::enable_shared_from_this<TcpClient> {
    friend TcpConnection;
    friend muduo::TcpClientPtr muduo::CreateTcpClient(EventLoop* loop, const InetAddr& server_addr, std::string name);
    
    // private constructor to prevent create the instance on stack
    explicit TcpClient(EventLoop* loop, const InetAddr& server_addr, std::string name);

public:
    /// @note When the TcpClient instance destroyed, the connection will also be closed
    ~TcpClient();

    /// @brief Starts to connect the specified server
    void Connect();

    /// @brief Shutdown the connection if the connection is connected now
    void Shutdown();

    /// @brief Stops to connect the specified server if is attempting to connect
    void Stop();

    const TcpConnectionPtr& GetConnection() const
    { return connection_; }

    const std::string& GetName() const
    { return clientName_; }

    EventLoop* GetEventLoop()
    { return loop_; }

    bool IsRetry() const
    { return retry_.load(std::memory_order::memory_order_relaxed); }

    /// Disable retry to connect by default
    void EnableRetry()
    { retry_.store(true, std::memory_order::memory_order_relaxed); }

    void SetConnectionCallback(const ConnectionCallback_t& cb)
    { connectionCb_ = cb; }
    void SetOnMessageCallback(const MessageCallback_t& cb)
    { onMessageCb_ = cb; }
    void SetWriteCompleteCallback(const WriteCompleteCallback_t& cb)
    { writeCompleteCb_ = cb; }

private:
    void HandleConnectSuccessfully(int sockfd);
    void HandleRemoveConnection(const TcpConnectionPtr& conn);

private:
    EventLoop* loop_;
    ConnectorPtr connector_;
    std::string clientName_;
    std::atomic_bool doConnect_ {false};
    std::atomic_bool retry_ {false};
    uint32_t nextConnId_ {1};
    TcpConnectionPtr connection_ {nullptr};
    std::mutex mutex_ {};   // for protecting connection
    ConnectionCallback_t connectionCb_ {DefaultConnectionCallback};
    MessageCallback_t onMessageCb_ {DefaultMessageCallback};
    WriteCompleteCallback_t writeCompleteCb_ {nullptr};
};

} // namespace muduo 

#endif // MUDUO_TCP_CLIENT_H
