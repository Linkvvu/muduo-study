#if !defined(MUDUO_CALLBACKS_H)
#define MUDUO_CALLBACKS_H

#include <functional>
#include <chrono>
#include <memory>

namespace muduo {
class EventLoop;        // forward declaration
class TcpConnection;    // forward declaration
class Buffer;           // forward declaration

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ReceiveTimePoint_t = std::chrono::system_clock::time_point;   // must same as EventLoop::ReceiveTimePoint_t
using PendingEventCb_t = std::function<void()>;

using IoThreadInitCallback_t = std::function<void(EventLoop* loop)>;

/// @brief Responsible for handling events which related to connection creation and destruction
using ConnectionCallback_t = std::function<void(const TcpConnectionPtr& conn)>;
// using CloseCallback_t = std::function<void(const TcpConnectionPtr& conn)>;  // replaced by ConnectionCallback_t
using MessageCallback_t = std::function<void(const TcpConnectionPtr& conn, Buffer* buf, ReceiveTimePoint_t)>;
using HighWaterMarkCallback_t = std::function<void (const TcpConnectionPtr&, size_t)>;
using WriteCompleteCallback_t = std::function<void (const TcpConnectionPtr&)>;

void DefaultConnectionCallback(const TcpConnectionPtr& conn);
void DefaultMessageCallback(const TcpConnectionPtr& conn, Buffer* buf, ReceiveTimePoint_t);
} // namespace muduo 

#endif // MUDUO_CALLBACKS_H
