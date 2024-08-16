#ifndef _AERO_NETWORKING_SOCKET_H_
#define _AERO_NETWORKING_SOCKET_H_

#include "aero/core/log.h"
#include <stdint.h>
#include <string>

#ifdef PLATFORM_WINDOWS
    #include <WS2tcpip.h>
    #include <WinSock2.h>
    #include <Windows.h>
/*typedef SOCKET Socket;*/
#else
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <unistd.h>
/*typedef int Socket;*/
typedef int SOCKET;
static const int INVALID_SOCKET = 0;
static const int SOCKET_ERROR   = -1;

#endif // PLATFORM_WINDOWS

namespace aero::networking
{

static int s_WSAInstances    = 0;
static bool s_WSAInitialized = false;

static int GetError()
{
#ifndef PLATFORM_WINDOWS
    return 0;
#endif
    return WSAGetLastError();
}

static bool InitializeNetworking()
{
#ifndef PLATFORM_WINDOWS
    return true;
#endif

    s_WSAInstances++;
    if (s_WSAInitialized)
        return true;

    LOG_DEBUG_TAG("Networking", "Running on Windows: Initialize WSA ...");
    WSADATA wsa_data;

    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
    {
        LOG_ERROR_TAG(
            "Networking", "Could not initialize WinSock. Error Code: {0}", WSAGetLastError()
        );
        s_WSAInstances--;
        return false;
    }

    LOG_DEBUG_TAG("Networking", "WinSock v{0} initiallized successfully!", wsa_data.wVersion);
    s_WSAInitialized = true;
    return true;
}

static void CleanupNetworking()
{

#ifndef PLATFORM_WINDOWS
    return;
#endif

    s_WSAInstances--;

    if (s_WSAInstances > 0)
        return;

    LOG_DEBUG_TAG("Networking", "No remaining dependants: Clean up WSA");
    WSACleanup();
    s_WSAInitialized = false;
}

enum class SocketType : uint16_t
{
    TCP = 0,
    UDP,
    _COUNT
};

static std::string SocketTypeName[(size_t)SocketType::_COUNT] = {"TCP", "UDP"};

struct Socket
{
    SOCKET socket = INVALID_SOCKET;
    sockaddr_in address;
};

static bool CreateSocket(Socket& s, SocketType type, uint16_t port)
{
    LOG_DEBUG_TAG("Networking", "Creating {} socket ...", SocketTypeName[(size_t)type]);

    if (type == SocketType::TCP && (s.socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        LOG_ERROR_TAG("Networking", "Could not create socket [ Error {} ]", GetError());
        return false;
    }
    if (type == SocketType::UDP && (s.socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        LOG_ERROR_TAG("Networking", "Could not create socket [ Error {} ]", GetError());
        return false;
    }

    if (s.socket == INVALID_SOCKET)
        return false;

    LOG_DEBUG_TAG(
        "Networking", "{} socket created! Initializing socket on port {}",
        SocketTypeName[(size_t)type], port
    );

    s.address.sin_family      = AF_INET;
    s.address.sin_port        = htons(port);
    s.address.sin_addr.s_addr = INADDR_ANY;

    return true;
}

static bool BindSocket(Socket& s)
{
    if (bind(s.socket, (sockaddr*)&s.address, sizeof(s.address)) < 0)
    {
        LOG_ERROR_TAG(
            "Networking", "Failed to bind socket to port {} [ Error {} ]", s.address.sin_port,
            GetError()
        );
        return false;
    }
    return true;
}

static void CloseSocket(Socket& s)
{
#ifndef PLATFORM_WINDOWS
    close(s.socket);
    return;
#else
    closesocket(s.socket);
    return;
#endif // PLATFORM_WINDOWS
}

} // namespace aero::networking
#endif // _AERO_NETWORKING_SOCKET_H_
