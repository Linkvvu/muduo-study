/// This is a internal header file, you should not include this.
/// This file encapsulates some operations of socket
#include <arpa/inet.h>
#include <cstdlib>

namespace muduo {
namespace net {
namespace sockets {
///
/// Creates a non-blocking socket file descriptor,
/// abort if any error.
extern int createNon_blockingOrDie();

extern void bindOrDie(const int sockfd, const struct ::sockaddr_in* addr);
extern void listenOrDie(const int sockfd);
extern int accept(const int sockfd, struct ::sockaddr_in* addr);
extern int connect(const int sockfd, const struct ::sockaddr_in* addr);
extern ssize_t read(const int sockfd, void* buf, std::size_t count);
extern ssize_t readv(const int sockfd, const struct ::iovec* iov, int count);
extern ssize_t write(const int sockfd, const void* buf, std::size_t count);
extern ssize_t writev(const int sockefd, const struct ::iovec* iov, int count);
extern void close(const int fd);
extern void shutdown_w(const int fd);
extern void to_format_ipAndPort(char* buf, std::size_t size, const struct ::sockaddr_in* addr);
extern void to_format_ip(char* buf, std::size_t size, const struct ::sockaddr_in* addr);
extern sockaddr_in from_ipAndPort(const char* ip, const uint16_t port);
extern int get_socket_error(int sockfd);
extern sockaddr_in get_local_addr(int sockfd);
extern sockaddr_in get_peer_addr(int sockfd);

} // namespace sockets 
} // namespace net 
} // namespace muduo 