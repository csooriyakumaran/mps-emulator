#include "aero/networking/server.h"

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

    LOG_INFO_TAG(m_ServerInfo.name, "Starting server on port {}", m_ServerInfo.port);

    if (!InitializeNetworking())
    {
        LOG_ERROR_TAG(
            m_ServerInfo.name, "Could not initialize Networking API. Aborting server startup."
        );
        return;
    }

    //- create TCP Socket
    if (!CreateSocket(m_ServerSocket, SocketType::TCP, m_ServerInfo.port))
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

    m_Threads.Enqueue([this]() { AcceptConnectionThreadFn(); }, "AcceptConnectionThreadFn");

    //- create UDP Socket
    if (!CreateSocket(m_StreamSocket, SocketType::UDP, m_ServerInfo.port))
    {
        LOG_ERROR_TAG(
            m_ServerInfo.name, "Could not create stream Socket. Aborting server startup."
        );

        CleanupNetworking();
        return;
    }
    m_Threads.Enqueue([this]() { DataStreamThreadFn(); }, "DataStreamThreadFn");
}

void Server::Stop()
{
    LOG_INFO_TAG(m_ServerInfo.name, "Stopping TCP server. closing port {}", m_ServerInfo.port);

    m_ServerIsRunning = false;

    m_Threads.StopAll();

    for (auto& [id, client] : m_Connections)
        KickClient(id);

    m_Connections.clear();

    CloseSocket(m_ServerSocket);
    CloseSocket(m_StreamSocket);
    CleanupNetworking();
}

void Server::AcceptConnectionThreadFn()
{
    while (m_ServerIsRunning)
    {
        LOG_INFO_TAG(m_ServerInfo.name, "Listening for connections on port {}", m_ServerInfo.port);
        if (!ListenForConnection(m_ServerSocket))
            continue;

        Socket conn;
        if (!AcceptConnection(m_ServerSocket, conn))
            continue;

        char ipstr[INET_ADDRSTRLEN];
        inet_ntop(conn.address.sin_family, &(conn.address.sin_addr), ipstr, INET_ADDRSTRLEN);

        auto& client = m_Connections[(uint64_t)conn.socket];
        client.id    = (uint64_t)conn.socket;
        client.ip    = std::string(ipstr);
        client.port  = conn.address.sin_port;

        LOG_INFO_TAG(
            m_ServerInfo.name, "Connection esbatlished with {}:{}", client.ip, client.port
        );
        m_Threads.Enqueue([this, client]() { HandleConnectionThreadFn(client.id); });

        if (m_ClientConnectCallback)
            m_ClientConnectCallback(client.id);
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

        LOG_DEBUG_TAG(
            m_ServerInfo.name, "Waiting for data from client at {}:{}", client.ip, client.port
        );

        //- blocking
        int bytes_read = recv((SOCKET)id, (char*)buf.data, (int)buf.size, 0);

        if (bytes_read < 0)
        {
            LOG_ERROR_TAG(m_ServerInfo.name, "Failed on recv: client id {}", id);
            break;
        }
        if (bytes_read == 0)
        {
            LOG_WARN_TAG(m_ServerInfo.name, "Client {} disconnected from the server", id);
            break;
        }

        if (m_DataReceivedCallback)
            m_DataReceivedCallback(id, Buffer::Copy(buf.data, bytes_read));
    }

    buf.Release();
    KickClient(id);
}

void Server::DataStreamThreadFn()
{

    std::function<bool(void)> is_not_empty = [this]()
    {
        this->m_StreamLock.lock();
        bool is_not_empty = !this->m_DataQueue.empty();
        this->m_StreamLock.unlock();
        return is_not_empty;
    };
    LOG_INFO_TAG(m_ServerInfo.name, "Starting UDP thread function, waiting for data to send");
    while (m_ServerIsRunning)
    {
        while (is_not_empty())
        {
            DataPacket packet = m_DataQueue.front();
            m_DataQueue.pop();

            if (packet.id == 0)
            {
                LOG_WARN_TAG(
                    m_ServerInfo.name,
                    "Cannot stream data from the console since it has no knowlege of the clients"
                );
                packet.data.Release();
                continue;
            }

            // if the target cleint has disconnected we want to disregard this packet
            if (m_Connections.find(packet.id) == m_Connections.end())
            {
                LOG_ERROR_TAG(
                    m_ServerInfo.name, "Cannot stream data: No registered client with id {}",
                    packet.id
                );
                packet.data.Release();
                continue;
            }

            ClientInfo& client = m_Connections[packet.id];
            struct sockaddr_in client_addr;
            client_addr.sin_family      = AF_INET;
            client_addr.sin_port        = htons(packet.port);
            client_addr.sin_addr.s_addr = inet_addr(client.ip.c_str());

            LOG_DEBUG_TAG(m_ServerInfo.name, "Streaming data to {}:{}", client.ip, packet.port);

            sendto(
                m_StreamSocket.socket, (char*)packet.data.data, (int)packet.data.size, 0,
                (struct sockaddr*)&client_addr, sizeof(client_addr)
            );

            packet.data.Release();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void Server::KickClient(uint64_t id)
{

    if (m_ClientDisconnectCallback)
        m_ClientDisconnectCallback(id);

    m_Connections.erase(id);
    CloseSocket(id);
}

void Server::SendBuffer(uint64_t id, Buffer buf)
{
    if (id == 0)
    {
        return SendBufferToAll(buf);
    }

    if (m_Connections.find(id) == m_Connections.end())
    {
        LOG_ERROR_TAG(m_ServerInfo.name, "No registered client with id {}", id);
        // buf.Release();
        return;
    }

    auto& client = m_Connections[id];

    send((SOCKET)client.id, (char*)buf.data, (int)buf.size, 0);

    // buf.Release();
    return;
}

void Server::SendBufferToAll(Buffer buf)
{
    for (auto& [id, client] : m_Connections)
        send((SOCKET)client.id, (char*)buf.data, (int)buf.size, 0);

    buf.Release();
}

void Server::SendString(uint64_t id, const std::string& str)
{
    SendBuffer(id, Buffer(str.data(), str.size()));
}

void Server::SendStringToAll(const std::string& str)
{
    SendBufferToAll(Buffer(str.data(), str.size()));
}

bool Server::IsClientConnected(uint64_t id) const
{
    return !(m_Connections.find(id) == m_Connections.end());
}

void Server::StreamData(uint64_t id, uint16_t port, Buffer buf)
{
    m_StreamLock.lock();
    m_DataQueue.push({id, port, aero::Buffer::Copy(buf.data, buf.size)});
    m_StreamLock.unlock();
    buf.Release();
}

} // namespace aero::networking
