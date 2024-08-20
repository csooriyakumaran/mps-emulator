#include "aero/networking/server.h"
#include <chrono>

#include "aero/core/log.h"

namespace aero::networking
{

Server::Server(const ServerInfo& info)
    : m_ServerInfo(info),
      m_Threads(info.workers)
{
}

Server::~Server() { Stop(); }

void Server::Start()
{

    LOG_INFO_TAG(
        m_ServerInfo.name, "Starting {} server on port {}",
        SocketTypeName[(size_t)m_ServerInfo.type], m_ServerInfo.port
    );

    if (!InitializeNetworking())
    {
        LOG_ERROR_TAG(
            m_ServerInfo.name, "Could not initialize Networking API. Aborting server startup."
        );
        return;
    }

    //- create Socket
    if (!CreateSocket(m_ServerSocket, m_ServerInfo.type, m_ServerInfo.port))
    {
        LOG_ERROR_TAG(m_ServerInfo.name, "Could not create Socket. Aborting server startup.");

        CleanupNetworking();
        return;
    }

    //- bind Socket
    if (!BindSocket(m_ServerSocket))
    {
        LOG_ERROR_TAG(m_ServerInfo.name, "Could not bind Socket. Aborting server startup.");
        CloseSocket(m_ServerSocket);
        CleanupNetworking();
        return;
    }

    LOG_DEBUG_TAG(m_ServerInfo.name, "Server started up sucessfully!");
    m_ServerIsRunning = true;

    if (m_ServerInfo.type == SocketType::TCP)
        m_Threads.Enqueue([this](){ AcceptConnectionThreadFn(); }, "AcceptConnectionThreadFn");
        /*m_Threads.Enqueue(*/
        /*    [this]() { TcpThreadFn(); },*/
        /*    std::format("{}: {}", m_ServerInfo.name, "Listening Thread")*/
        /*);*/

    /*m_Threads.Enqueue([this]() { SendDataThreadFn(); }, "Data Tranfer Thread");*/
}

void Server::Stop()
{
    LOG_INFO_TAG(
        m_ServerInfo.name, "Stopping {} server. closing port {}",
        SocketTypeName[(size_t)m_ServerInfo.type], m_ServerInfo.port
    );
    m_ServerIsRunning = false;
    m_ClientConnected = false;

    // for connected clients close sockets.

    CloseSocket(m_ServerSocket);
    CleanupNetworking();
}

void Server::AcceptConnectionThreadFn()
{
    while (m_ServerIsRunning)
    {
        LOG_INFO_TAG(m_ServerInfo.name, "Listening for connections on port {}", m_ServerInfo.port);
        if(!ListenForConnection(m_ServerSocket))
            continue;

        Socket conn;
        if(!AcceptConnection(m_ServerSocket, conn))
            continue;

        char ipstr[INET_ADDRSTRLEN];
        inet_ntop(
            conn.address.sin_family, &(conn.address.sin_addr), ipstr,
            INET_ADDRSTRLEN
        );

        auto& client = m_Connections[(uint64_t)conn.socket];
        client.id = (uint64_t)conn.socket;
        client.ip = std::string(ipstr);
        client.port = conn.address.sin_port;

        LOG_INFO_TAG(m_ServerInfo.name, "Connection esbatlished with {}:{}", client.ip, client.port);
        m_Threads.Enqueue([this, client](){ HandleConnectionThreadFn(client.id); });

    }
}

void Server::HandleConnectionThreadFn(uint64_t id)
{
    Buffer buf;
    buf.Allocate(m_ServerInfo.buffer_size);

    while (m_ServerIsRunning)
    {
        buf.Zero();
        //- are we still accepting messages from this socket?
        if (m_Connections.find(id) == m_Connections.end())
            break;

        auto& client = m_Connections[id];

        LOG_DEBUG_TAG(m_ServerInfo.name, "Waiting for data from client at {}:{}", client.ip, client.port); 

        //- blocking
        int bytes_read = recv((SOCKET)id, (char*)buf.data, (int)buf.size, 0);

        if (bytes_read < 0)
        {
            LOG_ERROR_TAG(m_ServerInfo.name, "Failed on recv");
            break;
        }
        if (bytes_read == 0)
        {
            LOG_WARN_TAG(m_ServerInfo.name, "Client disconnected from the server");
            return;
        }

        if (m_DataReceivedCallback)
            m_DataReceivedCallback(id, Buffer::Copy(buf.data, bytes_read));

    }

    buf.Release();
}


void Server::SendBuffer(uint64_t id, Buffer buf)
{
    if ( m_Connections.find(id) == m_Connections.end())
        return LOG_ERROR_TAG(m_ServerInfo.name, "No registered client with id {}", id);

    auto& client = m_Connections[id];

    send((SOCKET)client.id, (char*)buf.data, (int)buf.size, 0);
}
void Server::SendBufferToAll(Buffer buf)
{
    for (auto& [id, client] : m_Connections)
        send((SOCKET)client.id, (char*)buf.data, (int)buf.size, 0);
}

/*void Server::SendBuffer(Buffer buf)*/
/*{*/
/*    if (!m_ClientConnected)*/
/*    {*/
/*        LOG_ERROR_TAG(*/
/*            m_ServerInfo.name, "No clients connected. Cannot set msg `{}`", buf.As<char>()*/
/*        );*/
/*        return;*/
/*    }*/
/**/
/*    if (!m_ServerIsRunning)*/
/*    {*/
/*        LOG_ERROR_TAG(*/
/*            m_ServerInfo.name, "Server is not running. Cannot set msg `{}`", buf.As<char>()*/
/*        );*/
/*        return;*/
/*    }*/
/**/
/*    LOG_DEBUG_TAG(m_ServerInfo.name, "Attempting to send msg to client {}", buf.As<char>());*/
/**/
/*    if (send(m_ClientSocket.socket, (char*)buf.data, (int)buf.size, 0) == SOCKET_ERROR)*/
/*    {*/
/*        LOG_ERROR_TAG(*/
/*            m_ServerInfo.name,*/
/*            "Failed to send data. Client must have disconnected. Close the client socket"*/
/*        );*/
/*        CloseSocket(m_ClientSocket);*/
/*        m_ClientConnected = false;*/
/*        if (m_ClientDisconnectCallback)*/
/*            m_ClientDisconnectCallback();*/
/*    }*/
/*}*/

void Server::SendString(uint64_t id, const std::string& str) { SendBuffer(id, Buffer(str.data(), str.size())); }
void Server::SendStringToAll(const std::string& str) { SendBufferToAll(Buffer(str.data(), str.size())); }

} // namespace aero::networking
