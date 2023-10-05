#if !defined(MUDUO_CALLBACKS_H)
#define MUDUO_CALLBACKS_H

#include <functional>

namespace muduo {
class EventLoop;    // forward declaration

using PendingEventCb_t = std::function<void()>;
using IoThreadInitCb_t = std::function<void(EventLoop* loop)>;
} // namespace muduo 

#endif // MUDUO_CALLBACKS_H
