#if !defined(MUDUO_BASE_SOCKETOPS_H)
#define MUDUO_BASE_SOCKETOPS_H
#include <InetAddr.h>
#include <arpa/inet.h>
#include <cstddef>

namespace muduo {
namespace sockets {

/// create an non-blocking socket
/// abort if any error
extern int createNonblockingOrDie(sa_family_t family);     

extern void bindOrDie(int sockfd, const struct sockaddr* addr);

/**
 * @return if 0 is returned represents success,
 * if -1 is returned represents failure And TLS variable 'errno' is set
 */
extern int connect(int sockfd, const struct sockaddr* addr);
extern void listenOrDie(int sockfd);

/**
 * @brief Accept a incoming connection
 * 
 * @param listener sockfd of listener
 * @param addr address of incoming connection, 
 * 'struct sockaddr_in6' can represent both Ipv4 and Ipv6 addresses
 */
extern int accept(int listener, struct sockaddr_in6* addr);
extern ssize_t read(int sockfd, void* buf, size_t size);
extern ssize_t write(int sockfd, const void* buf, size_t size);
extern void close(int sockfd);
extern void shutdownWrite(int sockfd);

/// @return Return error code 
extern int getSocketError(int sockfd);

namespace address {

extern union sockets::address::SockAddr getLocalAddr(int sockfd);
extern union sockets::address::SockAddr getRemoteAddr(int sockfd);

/// @param buf result, If the buffer size is too small, truncate the resulting string
/// @param size size of buf
/// @param addr address
extern void toIp(char* buf, size_t size, const struct sockaddr* addr);
/// @param buf result, If the buffer size is too small, truncate the resulting string 
/// @param size size of buf
/// @param addr address
extern void toIpPort(char* buf, size_t size, const struct sockaddr* addr);
/// @param ip the character string of ip 
/// @param port port
/// @param addr result
extern void fromIpPort(const char* ip, uint16_t port, struct sockaddr_in* addr);
/// @param ip the character string of ip 
/// @param port port
/// @param addr result
extern void fromIpPort(const char* ip, uint16_t port, struct sockaddr_in6* addr);


                        /* addr convert helpers */
extern const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr);
extern struct sockaddr* sockaddr_cast(struct sockaddr_in* addr);
extern const struct sockaddr* sockaddr_cast(const struct sockaddr_in6* addr);
extern struct sockaddr* sockaddr_cast(struct sockaddr_in6* addr);

extern const struct sockaddr_in* convert_to_sockaddr_in(const struct sockaddr* addr);
extern const struct sockaddr_in6* convert_to_sockaddr_in6(const struct sockaddr* addr);

} // namespace address 

using namespace address;

} // namespace sockets
} // namespace muduo 

#endif // MUDUO_BASE_SOCKETOPS_H
