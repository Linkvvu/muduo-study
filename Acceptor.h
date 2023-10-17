#if !defined(MUDUO_ACCEPTOR_H)
#define MUDUO_ACCEPTOR_H
#include <InetAddr.h>
#include <functional>
#include <memory>
#include <atomic>

namespace muduo {
class EventLoop;    // forward declaration
class Socket;       // forward declaration
class Channel;      // forward declaration

class Acceptor {
    Acceptor(const Acceptor&) = delete;
    Acceptor& operator=(const Acceptor&) = delete;
public:
    using NewConnectionCallback_t = std::function<void(int sockfd, const InetAddr& addr)>; 

public:
    Acceptor(EventLoop* owner, const InetAddr& addr, bool reuse_port);
    ~Acceptor() noexcept;
    void Listen();

    const InetAddr& GetListeningAddr() const
    { return addr_; }

    bool Listening() const {
        // return listening_.test();    // since C++ 20
        return listening_.load();
    }

    void SetNewConnectionCallback(const NewConnectionCallback_t& cb) 
    { onNewConnectionCb_ = cb; }
    
    std::string GetIp() const { return addr_.GetIp(); }
    std::string GetIpPort() const { return addr_.GetIpPort(); }

private:
    void HandleNewConnection();

private:
    EventLoop* const owner_;
    InetAddr addr_;
    std::unique_ptr<Socket> listener_;
    std::unique_ptr<Channel> channel_;
    int idleFd_;    // file-descriptor holder
    // std::atomic_flag listening_;
    std::atomic_bool listening_;

    NewConnectionCallback_t onNewConnectionCb_;
};

} // namespace muduo 

#endif // MUDUO_ACCEPTOR_H
