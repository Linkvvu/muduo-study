#if !defined(MUDUO_NET_CALLBACKS_H)
#define MUDUO_NET_CALLBACKS_H

#include <functional>

namespace muduo {
namespace net {

using timerCallback_t = std::function<void()>;
using pendingCallback_t = std::function<void()>;

} // namespace net 
} // namespace muduo
#endif // MUDUO_NET_CALLBACKS_H
