#include <muduo/examples/shudu-resolver/resolver.h>

int main() {
    event_loop loop;

    inet_address ser_addr(8888);
    shudu_resolver server(&loop, ser_addr, 5);
    server.start();
    loop.loop();
}