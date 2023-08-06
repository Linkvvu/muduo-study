#ifndef MUDUO_EXAMPLES_SHUDU_RESOLVER_ALGORITHM_SHUDUKU_H
#define MUDUO_EXAMPLES_SHUDU_RESOLVER_ALGORITHM_SHUDUKU_H


#include <muduo/base/Types.h>

// FIXME, use (const char*, len) for saving memory copying.
extern muduo::string solveSudoku(const muduo::string& puzzle);
const int kCells = 81;

#endif  // MUDUO_EXAMPLES_SHUDU_RESOLVER_ALGORITHM_SHUDUKU_H