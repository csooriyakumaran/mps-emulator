#include "networking.h"

#include <iostream>

int main()
{
    mps::Server tcp;
    mps::Server udp;

    tcp.info = { "127.0.0.1", 65432, mps::SocketType::UDP, 4096 };

    udp.info = {
        "127.0.0.1",
        65433,
        mps::SocketType::UDP,
        348
    };

    std::cout << "Server socket type: " << mps::GetSocketType(&tcp) << std::endl;
    

    return 0;
}
