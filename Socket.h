#if !defined(MUDUO_SOCKET_H)
#define MUDUO_SOCKET_H

namespace muduo {

class InetAddr;     // forward declaration

/// @brief socket wrapper, owning socket handle
class Socket {
    // non-copyable & non-moveable
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    
public:
    explicit Socket(int sockfd)
        : sockfd_(sockfd)
    { }

    ~Socket() noexcept;

    int FileDescriptor() const
    { return sockfd_; }

    void Listen();
    void BindInetAddr(const InetAddr& addr);
    void SetReusePort(bool on);
    void SetReuseAddr(bool on);
    void SetTcpNoDelay(bool on);
    int Accept(InetAddr* addr);
    void ShutdownWrite();
        
private:
    const int sockfd_;
};

} // namespace muduo 

#endif // MUDUO_SOCKET_H
