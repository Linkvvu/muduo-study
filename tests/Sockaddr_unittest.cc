#include <arpa/inet.h>
#include <iostream>
#include <gtest/gtest.h>
#include <cstring>
#define BIND_SUCCESS 0
#define BIND_FAIL    -1

/// @note The actual structure passed for the addr argument will depend on the address family of socket.
/// @note 传递给addr参数的实际结构取决于socket的地址族。

TEST(Sockaddr, IPv4BindSockaddr_in) {
    int sockfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = 0;
    addr.sin_addr.s_addr = INADDR_ANY;

    ASSERT_EQ(::bind(sockfd, (sockaddr*)&addr, sizeof(sockaddr_in)), BIND_SUCCESS) << "failed to bind, detail: " << std::strerror(errno) << std::endl;
}

TEST(Sockaddr, IPv4BindSockaddr_in6) {
    int sockfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = 0;
    addr.sin_addr.s_addr = INADDR_ANY;

    ASSERT_EQ(::bind(sockfd, (sockaddr*)&addr, sizeof(sockaddr_in6)), BIND_SUCCESS) << "failed to bind, detail: " << std::strerror(errno) << std::endl;
}

TEST(Sockaddr, IPv6BindSockaddr_in) {
    int sockfd = ::socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in6 addr;
    addr.sin6_family = AF_INET6;
    addr.sin6_port = 0;
    inet_pton(AF_INET6, "::1", &addr.sin6_addr);


    EXPECT_EQ(::bind(sockfd, (sockaddr*)&addr, sizeof(sockaddr_in)), BIND_FAIL) << "failed to bind, detail: " << std::strerror(errno) << std::endl;
}

TEST(Sockaddr, IPv6BindSockaddr_in6) {
    int sockfd = ::socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in6 addr;
    addr.sin6_family = AF_INET6;
    addr.sin6_port = 0;
    inet_pton(AF_INET6, "::1", &addr.sin6_addr);


    EXPECT_EQ(::bind(sockfd, (sockaddr*)&addr, sizeof(sockaddr_in6)), BIND_SUCCESS) << "failed to bind, detail: " << std::strerror(errno) << std::endl;
}