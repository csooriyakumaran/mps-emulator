#ifndef _AERO_NETWORKING_H_
#define _AERO_NETWORKING_H_

#include <functional>
#include <map>
#include <stdint.h>
#include <string>

#include "aero/core/buffer.h"
#include "aero/networking/socket.h"

namespace aero::networking
{

typedef uint32_t ClientID;

struct ClientInfo
{
    std::string host     = "127.0.0.1";
    uint16_t port        = 65432;
    SocketType type      = SocketType::TCP;
    uint32_t buffer_size = 4096;
};

class TcpServer
{
public:
    using DataReceivedCallback     = std::function<void(const ClientInfo&, const Buffer)>;
    using ClientConnectCallback    = std::function<void(const ClientInfo&)>;
    using ClientDisconnectCallback = std::function<void(const ClientInfo&)>;

public:
    TcpServer() {}
    ~TcpServer() {}

    void Start();
    void Stop();

    void SendBufferToClient(ClientID client, Buffer buff);
    void SendBufferToAll(Buffer buff, ClientID exclude = 0);

    template<typename T>
    void SendDataToClient(ClientID client, const T& data)
    {
        SendBufferToClient(client, Buffer(&data, sizeof(T)));
    }

    template<typename T>
    void SendDataToAll(const T& data, ClientID exclude)
    {
        SendBufferToAll(Buffer(&data, sizeof(T)), exclude);
    }



    void KickClient(ClientID client);

    bool IsRunning() const { return m_running; }

private:
    void RecvData();
    void AcceptClients();
    void HandleConnection();

private:
    std::map<ClientID, ClientInfo> m_Clients;
    bool m_running = false;
    SOCKET socket  = 0u;

    DataReceivedCallback m_DataReceivedCallback;
    ClientConnectCallback m_ClientConnectCallback;
    ClientDisconnectCallback m_ClientDisconnectCallback;
};

} // namespace aero::networking

#endif // _AERO_NETWORKING_H_
