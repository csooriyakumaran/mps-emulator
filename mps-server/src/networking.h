#ifndef _NETWORKING_H_
#define _NETWORKING_H_
#include <stdint.h>
#include <string>

#ifdef PLATFORM_WINDOWS
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <Windows.h>
#endif // PLATFORM_WINDOWS

namespace mps
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

struct Server
{
        ServerInfo info      = {};
        bool running         = false;
        SOCKET socket;
};

void StartServer(Server *server);
void StopServer(Server *server);

void AcceptClient(Server *server);
void HandleConnection(Server *server);

std::string GetSocketType(Server *server) { return SocketTypes[(size_t)server->info.type]; }

} // namespace mps

#endif // _NETWORKING_H_
