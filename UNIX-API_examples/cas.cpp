#include <atomic>
#include <iomanip>
#include <iostream>
std::atomic<int> a(10);

int main() {
    int expected = 10;
    auto ok = a.compare_exchange_strong(expected, 100);
    std::cout << std::boolalpha << ok << std::endl;
    std::cout << expected << " " << a << std::endl;
}