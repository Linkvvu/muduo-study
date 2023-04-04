#include <muduo/base/Thread.h>
#include <stdio.h>
#include <iostream>

int main() {
    auto x = printf("%2d", 100000);
    std::cout << std::endl;
    std::cout << x << std::endl;
}