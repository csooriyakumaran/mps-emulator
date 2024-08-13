#ifndef _AERO_NETWORKING_H_
#define _AERO_NETWORKING_H_

#include <functional>
#include <stdint.h>
#include <string>

#include "aero/core/buffer.h"
#include "aero/networking/socket.h"

namespace aero::networking
{

enum class SocketType : uint16_t
{
    TCP = 0,
    UDP,
    _COUNT
};

static std::string SocketTypes[(size_t)SocketType::_COUNT] = {"TCP", "UDP"};

struct ServerInfo
{
    std::string host     = "127.0.0.1";
    uint16_t port        = 65432;
    SocketType type      = SocketType::TCP;
    uint32_t buffer_size = 4096;
};

class TcpServer
{
    public:
    using DataReceivedCallback     = std::function<void(const ServerInfo&, const Buffer)>;
    using ClientConnectCallback    = std::function<void(const ServerInfo&)>;
    using ClientDisconnectCallback = std::function<void(const ServerInfo&)>;

    public:
    TcpServer() {}
    ~TcpServer() {}

    void Start();
    void Stop();
    void SendData();
    void RecvData();
    void AcceptClients();
    void HandleConnection();

    std::string GetSocketType() { return SocketTypes[(size_t)info.type]; }

    private:
    ServerInfo info = {};
    bool running    = false;
    SOCKET socket   = 0u;
};

} // namespace aero::networking

#endif // _AERO_NETWORKING_H_
