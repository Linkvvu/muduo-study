#ifndef MUDUO_BASE_EXCEPTION_H
#define MUDUO_BASE_EXCEPTION_H

#include <exception>
#include <string>

namespace muduo {
    
class Exception : public std::exception {
public:
    Exception(const std::string& what, bool demangle = false);
    ~Exception() noexcept override = default;

    const char* what() const noexcept override {
        return message_.c_str();
    }

    const char* stackTrace() const noexcept {
        return stack_.c_str();
    }
private:
    void fillStackTrace(bool demangle = false);
    std::string demangle(const char* symbol) const;
private:
    std::string message_;
    std::string stack_;
};
}  // namespace muduo
#endif  // MUDUO_BASE_EXCEPTION_H