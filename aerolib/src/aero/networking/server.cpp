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

    m_Threads.Enqueue(
        [this]() { NetworkThreadFn(); },
        std::format("{}: {}", m_ServerInfo.name, "Listening Thread")
    );

    m_Threads.Enqueue([this]() { SendDataThreadFn(); }, "Data Tranfer Thread");
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

void Server::NetworkThreadFn()
{
    m_ServerIsRunning = true;
    //- as long as the server is running, keep the socket open for new connections
    while (m_ServerIsRunning)
    {

        if (!ListenForConnection(m_ServerSocket))
        {
            LOG_ERROR_TAG(m_ServerInfo.name, "Error while listening");
            return;
        }

        LOG_INFO_TAG(m_ServerInfo.name, "Listening for connection on port {}", m_ServerInfo.port);

        if (!AcceptConnection(m_ServerSocket, m_ClientSocket))
        {
            LOG_ERROR_TAG(m_ServerInfo.name, "Error while accepting");
            return;
        }

        //- register connection
        char ipstr[INET_ADDRSTRLEN];
        inet_ntop(
            m_ClientSocket.address.sin_family, &(m_ClientSocket.address.sin_addr), ipstr,
            INET_ADDRSTRLEN
        );

        LOG_INFO_TAG(
            m_ServerInfo.name, "Client connection established from {}:{}", ipstr,
            m_ClientSocket.address.sin_port
        );

        m_ClientConnected = true;

        if (m_ClientConnectCallback)
            m_ClientConnectCallback();

        while (m_ClientConnected)
        {
            PollData();
            PollConnectionStatus();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    LOG_DEBUG_TAG(m_ServerInfo.name, "Finishing NetworkThreadFn");
}

void Server::PollData()
{
    Buffer buf;
    buf.Allocate(m_ServerInfo.buffer_size);
    buf.Zero();

    int offset = 0;
    while (m_ClientConnected)
    {

        struct timeval tv = {0, 1000};
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(m_ClientSocket.socket, &rfds);

        int rec = select((int)m_ClientSocket.socket + 1, &rfds, NULL, NULL, &tv);

        if (rec < 0)
        {
            LOG_ERROR_TAG(m_ServerInfo.name, "Failed to recievedata");
            m_ClientConnected = false;
            buf.Release();
            break;
        }

        if (rec == 0)
            break;

        int bytes_read =
            recv((int)m_ClientSocket.socket, (char*)buf.data + offset, buf.size - offset, 0);

        if (bytes_read < 0)
        {
            LOG_ERROR_TAG(m_ServerInfo.name, "Failed on rev");
            m_ClientConnected = false;
            CloseSocket(m_ClientSocket);
            buf.Release();
            return;
        }
        if (bytes_read == 0)
        {
            LOG_WARN_TAG(m_ServerInfo.name, "Client disconnected from the server");
            m_ClientConnected = false;
            CloseSocket(m_ClientSocket);
            buf.Release();
            return;
        }

        offset += bytes_read;
    }

    if (offset == 0)
    {
        buf.Release();
        return;
    }

    if (m_DataReceivedCallback)
        m_DataReceivedCallback(Buffer::Copy(buf.data, offset));

    buf.Release();
    return;
}

void Server::PollConnectionStatus()
{
    if (m_ServerInfo.type == SocketType::UDP)
        return;
}

void Server::SendDataThreadFn()
{

    while (m_ServerIsRunning)
    {
        while (!m_DataQueue.empty())
        {
            Buffer buf = m_DataQueue.front();
            if (send(m_ClientSocket.socket, (char*)buf.data, buf.size, 0) == SOCKET_ERROR)
            {
                LOG_ERROR_TAG(
                    m_ServerInfo.name,
                    "Failed to send data. Client must have disconnected. Close the client socket"
                );
                CloseSocket(m_ClientSocket);
                m_ClientConnected = false;
                if (m_ClientDisconnectCallback)
                    m_ClientDisconnectCallback();
            }
            m_DataQueue.pop();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void Server::SendBuffer(Buffer buf) { m_DataQueue.push(buf); }

void Server::SendString(const std::string& str) { SendBuffer(Buffer(str.data(), str.size())); }

} // namespace aero::networking
