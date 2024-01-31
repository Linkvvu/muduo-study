#if !defined(MUDUO_CONNECTOR_H)
#define MUDUO_CONNECTOR_H

#include <muduo/EventLoop.h>
#include <muduo/InetAddr.h>

namespace muduo {

/// Responsible for the connecting operation
/// must be managed by @c std::shared_ptr
class Connector : public std::enable_shared_from_this<Connector> {
    Connector(const Connector&) = delete;
    Connector& operator=(const Connector&) = delete;

    enum State {
        kDisconnected,  // No connection operation is currently being performed
        kConnecting,    // Is waiting a async connect operation now
        kConnected      // Had successfully connected
    };

public: 
    using RetryDelayMs = std::chrono::milliseconds;
    using ConnectSuccessfullyCallback = std::function<void(int sockfd)>;
    static const RetryDelayMs kInitRetryDelayMs;
    static const RetryDelayMs kMaxRetryDelayMs;

    explicit Connector(EventLoop* loop, const InetAddr& server_addr);
    
    ~Connector() noexcept
    { assert(!chan_); }

    /// @brief Starts to thread-safely connect the specified server
    void Start();
    
    /// @brief Reattempts to connect the specified server
    /// @note Must be invoked in the loop-thread
    void Restart();

    /// @brief Stops to connect the specified server
    void Stop()
    { connect_.store(false, std::memory_order::memory_order_release); }

    void SetConnectSuccessfullyCallback(const ConnectSuccessfullyCallback& cb)
    { cb_ = cb; }
    
    const InetAddr& GetServerAddress() const
    { return serverAddr_; }

private:
    void StartInLoop();
    void DoConnect();
    void InitConnectOp(int sockfd);

    void HandleWrite();
    void HandleAsyncError();
    int RemoveAndResetChannel();

    void Retry();

private:
    EventLoop* loop_;
    InetAddr serverAddr_;
    State opState_ {State::kDisconnected};
    std::atomic_bool connect_ {false};
#ifdef MUDUO_USE_MEMPOOL
    std::unique_ptr<Channel, std::function<void(Channel*)>> chan_ { nullptr, [](Channel* ptr) {::delete ptr;} };
#else
    std::unique_ptr<Channel> chan_ {nullptr};
#endif
    ConnectSuccessfullyCallback cb_;
    RetryDelayMs retryDelayMs_ {kInitRetryDelayMs};
};

} // namespace muduo 

#endif // MUDUO_CONNECTOR_H
