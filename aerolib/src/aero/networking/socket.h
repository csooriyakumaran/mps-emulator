#ifndef _AERO_NETWORKING_SOCKET_H_
#define _AERO_NETWORKING_SOCKET_H_

#ifdef PLATFORM_WINDOWS
    #include <WS2tcpip.h>
    #include <WinSock2.h>
    #include <Windows.h>
typedef SOCKET Socket;

#else
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <unistd.h>
typedef int Socket;
static const int INVALID_SOCKET = -1;
static const int SOCKET_ERROR   = -1;

#endif // PLATFORM_WINDOWS

namespace aero::networking
{


enum class SocketType : uint16_t
{
    TCP = 0,
    UDP,
    _COUNT
};

static std::string SocketTypeName[(size_t)SocketType::_COUNT] = {"TCP", "UDP"};

} // namespace aero::networking
#endif // _AERO_NETWORKING_SOCKET_H_
