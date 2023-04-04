#include "muduo/base/Exception.h"
#include <execinfo.h> // for backtrace backtrace_symbols
#include <cxxabi.h>   // for abi::__cxa_demangle

muduo::Exception::Exception(const std::string& what, bool demangle) : message_(what) {
    fillStackTrace(demangle);
}

void muduo::Exception::fillStackTrace(bool demangle) {
    const int len = 200; // 最多保存200个栈帧
    void* buffer[len];
    int nptrs = ::backtrace(buffer, len);
    char** strs = ::backtrace_symbols(buffer, nptrs);
    if (strs) {
        for (int i = 0; i < nptrs; i++) {
            if (demangle) {
                auto human_fmt = this->demangle(strs[i]);
                stack_.append(human_fmt);
                stack_.push_back('\n');
            } else {
                stack_.append(strs[i]);
                stack_.push_back('\n');
            }
        }
        ::free(strs);
    }
}

std::string muduo::Exception::demangle(const char* symbol) const {
    size_t size;
    int status;
    char temp[128];
    char* demangled;
    // first, try to demangle a c++ name
    if (1 == sscanf(symbol, "%*[^(]%*[^_]%127[^)+]", temp)) {
        if (nullptr != (demangled = abi::__cxa_demangle(temp, NULL, &size, &status))) {
            std::string result(demangled);
            free(demangled);
            return result;
        }
    }
    // if that didn't work, try to get a regular c symbol
    if (1 == sscanf(symbol, "%127s", temp)) {
        return temp;
    }
    
    // if all else fails, just return the symbol
    return symbol;
}
