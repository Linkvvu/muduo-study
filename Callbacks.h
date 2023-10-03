#if !defined(MUDUO_CALLBACKS_H)
#define MUDUO_CALLBACKS_H

#include <functional>

namespace muduo {
using PendingEventCb_t = std::function<void()>;
} // namespace muduo 

#endif // MUDUO_CALLBACKS_H
