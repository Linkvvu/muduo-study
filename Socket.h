#if !defined(MUDUO_SOCKET_H)
#define MUDUO_SOCKET_H

namespace muduo {

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


private:
    const int sockfd_;
};

} // namespace muduo 

#endif // MUDUO_SOCKET_H
