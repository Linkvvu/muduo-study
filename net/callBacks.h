#if !defined(MUDUO_NET_CALLBACKS_H)
#define MUDUO_NET_CALLBACKS_H

#include <functional>
#include <memory>

namespace muduo {
namespace net {

class tcp_connection;   // forward declaration

using timerCallback_t = std::function<void()>;
using pendingCallback_t = std::function<void()>;

// for tcp-server
using onConnectionCallback_t = std::function<void(const std::shared_ptr<tcp_connection>&)>;
using onMsgCallback_t = std::function<void(const std::shared_ptr<tcp_connection>&, char*, std::size_t size)>;
using onCloseCallback_t = std::function<void(const std::shared_ptr<tcp_connection>&)>;

} // namespace net 
} // namespace muduo
#endif // MUDUO_NET_CALLBACKS_H
