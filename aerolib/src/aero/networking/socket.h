#ifndef _AERO_NETWORKING_SOCKET_H_
#define _AERO_NETWORKING_SOCKET_H_

#ifdef PLATFORM_WINDOWS
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <Windows.h>
#else
typedef int SOCKET;
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
static const int INVALID_SOCKET = -1;
static const int SOCKET_ERROR   = -1;
#endif // PLATFORM_WINDOWS

namespace aero::networking
{

struct Socket
{
    SOCKET sock = 0;

};

} // namspace aero::networking

#endif // _AERO_NETWORKING_SOCKET_H_

