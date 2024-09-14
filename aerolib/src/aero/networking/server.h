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
#include <mutex>

#include "aero/core/buffer.h"
#include "aero/core/threads.h"
#include "aero/networking/socket.h"

namespace aero::networking
{

struct ClientInfo
{
    uint64_t id;
    std::string ip;
    uint16_t port;
};

struct ServerInfo
{
    std::string name     = "Server";
    size_t workers       = 1;
    uint16_t port        = 0u;
    uint32_t buffer_size = 4096;
};

struct DataPacket
{
    uint64_t id;
    uint16_t port;
    aero::Buffer data;
};

class Server
{
public:
    using DataReceivedFn     = std::function<void(uint64_t id, const Buffer)>;
    using ClientConnectFn    = std::function<void(uint64_t id)>;
    using ClientDisconnectFn = std::function<void(uint64_t id)>;

public:
    Server(const ServerInfo& info);
    ~Server();

    void Start(); // setup the server socket
    void Stop();  // close any connected sockets, and close server socket

    //---- S E T - C A L L B A C K S ------------------------------------------

    void SetDataRecvCallback(const DataReceivedFn& fn) { m_DataReceivedCallback = fn; }
    void SetClientConCallback(const ClientConnectFn& fn) { m_ClientConnectCallback = fn; }
    void SetClientDisconCallback(const ClientDisconnectFn& fn) { m_ClientDisconnectCallback = fn; }

    //---- S E N D - D A T A --------------------------------------------------

    void SendBuffer(uint64_t id, Buffer buff);
    void SendString(uint64_t id, const std::string& msg);
    void SendBufferToAll(Buffer buff);
    void SendStringToAll(const std::string& msg);

    template<typename T>
    void SendData(uint64_t id, const T& data)
    {
        SendBuffer(id, Buffer(&data, sizeof(T)));
    }
    template<typename T>
    void SendDataToAll(const T& data)
    {
        SendBufferToAll(Buffer(&data, sizeof(T)));
    }

    void StreamData(uint64_t id, uint16_t port, Buffer buf);
    //------------------------------------------------------------------------
    void KickClient(uint64_t id);

    bool IsRunning() const { return m_ServerIsRunning; }
    bool IsClientConnected(uint64_t id) const;

private:
    void AcceptConnectionThreadFn();
    void HandleConnectionThreadFn(uint64_t id);
    void DataStreamThreadFn();

private:
    ServerInfo m_ServerInfo;

    std::map<uint64_t, ClientInfo> m_Connections;
    std::queue<DataPacket> m_DataQueue;
    bool m_ServerIsRunning = false;
    Socket m_ServerSocket;
    Socket m_StreamSocket;

    DataReceivedFn m_DataReceivedCallback;
    ClientConnectFn m_ClientConnectCallback;
    ClientDisconnectFn m_ClientDisconnectCallback;

    ThreadPool m_Threads;
    std::mutex m_StreamLock;
};

} // namespace aero::networking

#endif // _AERO_NETWORKING_SERVER_H_
