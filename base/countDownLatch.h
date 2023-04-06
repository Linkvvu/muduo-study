#if !defined(MUDUO_BASE_COUNTDOWNLATCH_H)
#define MUDUO_BASE_COUNTDOWNLATCH_H

#include <muduo/base/condition_variable.h>

namespace muduo {
    
class countDown_latch : private boost::noncopyable {
public:
    explicit countDown_latch(const int count);
    ~countDown_latch() = default;
    
    void wait();

    void countdown();

    int getCount() const;

private:
    // 注意：mutex_与cv_的顺序不能颠倒，因为构造cv_必须先构造mutex_，且initialize_list是按照声明顺序进行初始化的
    mutable mutexLock mutex_;   // 组合关系：本类对象需负责管理组合体的生命周期
    condition_variable cv_;     // 组合关系：本类对象需负责管理组合体的生命周期
    int count_;
};

} // namespace muduo 


#endif // MUDUO_BASE_COUNTDOWNLATCH_H
