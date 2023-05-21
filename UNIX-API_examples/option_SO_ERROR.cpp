#include <fcntl.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <error.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <stdio.h>
#include <unistd.h>

/// 当一个NON-Block的socket建立连接时，若连接不能立即被建立，则connect(3)会返回-1, errno被设为"EINPROGRESS"
/// 这种情况下，该连接将会由内核异步建立
/// 当连接建立成功或发生错误时，套接字会变得"可读|可写"
/// 为得知非阻塞套接字connect(3)是否完成, 我们需要通过"可读|可写"事件来判断（"可读|可写"事件并不代表连接建立成功, 仅表示connect(3)完成！）
/// 当"可读|可写"事件被触发，我们则可以通过 getsockopt(3) 和 SO_ERROR 得知连接是否建立成功，或连接建立失败的原因

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    // Set the socket to non-blocking mode
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in serv_addr {0};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);
    inet_pton(AF_INET, "192.168.1.1", &serv_addr.sin_addr);

    // Call connect() on the non-blocking socket
    int connect_result = connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    if (connect_result < 0) {
        perror("connect");
        if (errno != EINPROGRESS) {
            exit(EXIT_FAILURE);
        }
    }

    fd_set writefds;
	fd_set readfds;
    struct timeval timeout;
    int ret;

    FD_ZERO(&writefds);
	FD_ZERO(&readfds);
    FD_SET(sockfd, &writefds);
	FD_SET(sockfd, &readfds);
    timeout.tv_sec = 5; // 5 seconds timeout
    timeout.tv_usec = 0;

    ret = select(sockfd + 1, &readfds, &writefds, NULL, &timeout);

    if (ret > 0 && FD_ISSET(sockfd, &readfds) && FD_ISSET(sockfd, &writefds)) {
        int err;
        socklen_t len = sizeof(err);
        if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &len) < 0) {
            perror("getsockopt");
            exit(EXIT_FAILURE);
        }

        if (err) {
            fprintf(stderr, "SO_ERROR: %s\n", std::strerror(err));
            exit(EXIT_FAILURE);
        } else {
            printf("Connection established successfully\n");
        }
    } else if (ret == 0) {
        fprintf(stderr, "Connection timed out\n");
        exit(EXIT_FAILURE);
    } else {
        perror("select");
        exit(EXIT_FAILURE);
    }
}
