#include <muduo/examples/file-transmitter/transmitter.h>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc == 1) {
        std::cerr << "Usage: " << argv[0] << " <file_for_downloading>" << std::endl;
        return -1;
    }

    event_loop loop;
    inet_address ser_addr(8888);
    transmit_server transmitter(&loop, ser_addr, argv[1]);

    transmitter.start();
    loop.loop();    

    return 0;    
}