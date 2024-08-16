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

Server::~Server() {}

void Server::Start()
{

    LOG_INFO_TAG(
        m_ServerInfo.name, "Starting {} server on port {}",
        SocketTypeName[(size_t)m_ServerInfo.type], m_ServerInfo.port
    );

    if (!InitializeNetworking())
    {
        LOG_ERROR_TAG(m_ServerInfo.name, "Could not initialize Networking API. Aborting server startup.");
        return;
    }

    //- create Socket
    if (!CreateSocket(m_Socket, m_ServerInfo.type, m_ServerInfo.port))
    {
        LOG_ERROR_TAG(m_ServerInfo.name, "Could not create Socket. Aborting server startup.");
        return;
    }

    //- bind Socket
    if (!BindSocket(m_Socket))
    {
        LOG_ERROR_TAG(m_ServerInfo.name, "Could not bind Socket. Aborting server startup.");
        return;
    }

    LOG_DEBUG_TAG(m_ServerInfo.name, "Server started up sucessfully!");
    m_Running = true;



    //- start listening thread
    m_Threads.Enqueue([this]() { AcceptClientsThreadFn(); }, "Listening Thread");

    m_Threads.Enqueue([this]() { SendDataThreadFn(); }, "Data Tranfer Thread");
}

void Server::Stop()
{
    LOG_INFO_TAG(
        m_ServerInfo.name, "Stopping {} server. closing port {}",
        SocketTypeName[(size_t)m_ServerInfo.type], m_ServerInfo.port
    );
    m_Running = false;

    CloseSocket(m_Socket);
    WSACleanup();
}

void Server::SendBufferToAll(Buffer buff, ClientID exclude)
{
    for (const auto& [id, client] : m_Clients)
    {
        if (id != exclude)
            SendBufferToClient(id, buff);
    }
}
void Server::SendBufferToClient(ClientID client, Buffer buff) {}

void Server::AcceptClientsThreadFn()
{
    m_Running = true;
    LOG_DEBUG_TAG(m_ServerInfo.name, "Running NetworkThreadFn");
    while (m_Running)
    {
        //- accept client if on TCP
        //- spin that off to it's own thread
        //- for UDP just listen for data
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    LOG_DEBUG_TAG(m_ServerInfo.name, "Finishing NetworkThreadFn");
}
void Server::HandleConnectionThreadFn() {}
void Server::SendDataThreadFn() {

    while (m_Running)
    {
        while(!m_SendBufferQueue.empty())
        {
            Transmittal t = m_SendBufferQueue.front();
            SendBufferToClient(t.id, t.data);
            m_SendBufferQueue.pop();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

}

} // namespace aero::networking
