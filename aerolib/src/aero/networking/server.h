#ifndef _AERO_NETWORKING_SERVER_H_
#define _AERO_NETWORKING_SERVER_H_
/* ----------------------------------------------------------------------------
    AEROLIB
    -------
    aero/netowrking/server.h

    The server class is an abstraction around the system calls for networking
    sockets. Runs an listening thread to accept multiple clients, and send and
    receive data from all.


    Author:         C.Sooriyakumaran
    Change log:
                    [15-08-2024] Initial Release

/ -------------------------------------------------------------------------- */
#include <functional>
#include <map>
#include <stdint.h>
#include <string>

#include "aero/core/buffer.h"
#include "aero/core/threads.h"
#include "aero/networking/socket.h"

typedef uint32_t ClientID;

namespace aero::networking
{

struct ClientInfo
{
    std::string host     = "127.0.0.1";
    uint16_t port        = 65432;
    SocketType type      = SocketType::TCP;
    uint32_t buffer_size = 4096;
};

struct ServerInfo
{
    std::string name     = "Server";
    size_t workers       = 1;
    uint16_t port        = 0u;
    SocketType type      = SocketType::TCP;
    uint32_t buffer_size = 4096;
};

struct Transmittal
{
    ClientID id;
    Buffer data;

};
class Server
{
public:
    using DataReceivedFn     = std::function<void(const ClientInfo&, const Buffer)>;
    using ClientConnectFn    = std::function<void(const ClientInfo&)>;
    using ClientDisconnectFn = std::function<void(const ClientInfo&)>;

public:
    Server(const ServerInfo& info);
    ~Server();

    void Start();
    void Stop();

    //---- S E T - C A L L B A C K S ------------------------------------------

    void SetDataRecvCallback(const DataReceivedFn& fn) { m_DataReceivedCallback = fn; }
    void SetClientConCallback(const ClientConnectFn& fn) { m_ClientConnectCallback = fn; }
    void SetClientDisconCallback(const ClientDisconnectFn& fn) { m_ClientDisconnectCallback = fn; }

    //---- S E N D - D A T A --------------------------------------------------

    void SendBufferToClient(ClientID id, Buffer buff);
    void SendBufferToAll(Buffer buff, ClientID exclude = 0);
    void SendStringToClient(ClientID id, const std::string& msg);
    void SendStringToAll(const std::string& msg, ClientID exclude = 0);

    template<typename T>
    void SendDataToClient(ClientID client, const T& data)
    {
        SendBufferToClient(client, Buffer(&data, sizeof(T)));
    }

    template<typename T>
    void SendDataToAll(const T& data, ClientID exclude = 0)
    {
        SendBufferToAll(Buffer(&data, sizeof(T)), exclude);
    }

    //------------------------------------------------------------------------
    void KickClient(ClientID client);

    bool IsRunning() const { return m_Running; }

private:
    void SendDataThreadFn();

    void RecvData();
    void AcceptClientsThreadFn();
    void HandleConnectionThreadFn();

private:
    ServerInfo m_ServerInfo;

    std::map<ClientID, ClientInfo> m_Clients;
    std::queue<Transmittal> m_SendBufferQueue;
    bool m_Running  = false;
    Socket m_Socket = 0u;

    DataReceivedFn m_DataReceivedCallback;
    ClientConnectFn m_ClientConnectCallback;
    ClientDisconnectFn m_ClientDisconnectCallback;

    ThreadPool m_Threads;
};

} // namespace aero::networking

#endif // _AERO_NETWORKING_SERVER_H_
