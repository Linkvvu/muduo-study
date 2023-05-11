// This is a public header file, it must only include public header files.
#if !defined(MUDUO_NET_TIMERID_H)
#define MUDUO_NET_TIMERID_H

#include <cstdlib>

namespace muduo {
namespace net {

class timer;
    
class timer_id : muduo::copyable {
public:
    explicit timer_id(timer* const handle, u_int64_t sequence) : handle_(handle), seq_(sequence) {}
    timer* get_timer() { return handle_; }
    const timer* get_timer() const { return handle_; }
    uint64_t get_sequence() const { return seq_; }
    
private:
    timer* handle_; // 聚合关系
    u_int64_t seq_;
};

} // namespace net 
} // namespace muduo 
#endif // MUDUO_NET_TIMERID_H
