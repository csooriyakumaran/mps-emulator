#include "server-layer.h"
#include <chrono>
#include <format>
#include <iostream>

#include "aero/core/application.h"
#include "aero/core/log.h"
#include "aero/core/assert.h"

//- Initialize members
ServerLayer::ServerLayer(uint16_t port)
    : m_Port(port)
{
}

ServerLayer::~ServerLayer() {}

void ServerLayer::OnAttach()
{
    m_Console = std::make_unique<Console>();
    m_Console->SetMessageCallback(
        [this](std::string_view msg)
        {
            /*std::string cmd = std::format(R"({}\r\n)", msg);*/
            this->OnCommand(msg);
        }
    );

    //-set up the TCP server
    aero::networking::ServerInfo tcp_info;
    tcp_info.name    = "TCP-SERVER";
    tcp_info.workers = 5;
    tcp_info.port    = m_Port;
    tcp_info.buffer_size  = 1024;

    m_TCP            = std::make_unique<aero::networking::Server>(tcp_info);
    m_TCP->SetClientConCallback([this](){ OnClientConnected(); });
    m_TCP->SetDataRecvCallback([this](const aero::Buffer buf){ OnDataReceived(buf); });
    m_TCP->Start();

    /*aero::networking::ServerInfo udp_info;*/
    /*udp_info.name    = "UDP-SERVER";*/
    /*udp_info.workers = 1;*/
    /*udp_info.port    = m_Port;*/
    /*udp_info.type    = aero::networking::SocketType::UDP;*/
    /**/
    /*m_UDP            = std::make_unique<aero::networking::Server>(udp_info);*/
    /*m_UDP->Start();*/
}

void ServerLayer::OnDetach()
{

    m_TCP->Stop();
    /*m_UDP->Stop();*/
}

void ServerLayer::OnUpdate()
{
    LOG_DEBUG_TAG("ServerLayer", "Running at {:8.2f} fps", aero::Application::Get().FrameRate());
}

void ServerLayer::SendMsg(std::string_view msg)
{
    if (msg[0] == '/')
    {
        OnCommand(msg);
        return;
    }

    if (msg.empty())
        return;

    //- echo message to server console
    LOG_INFO_TAG("SERVER", msg);
}

void ServerLayer::OnCommand(std::string_view cmd)
{
    if (cmd.size() < 2 || cmd[0] != '/')
        return;

    std::string_view cmdStr(&cmd[1], cmd.size() - 1);

    if (cmdStr == "shutdown")
        return aero::Application::Get().Shutdown();
    if (cmdStr == "restart")
        return aero::Application::Get().Restart();

    //- TODO(send this to the client that requested it and echo to console)
    LOG_ERROR_TAG("SERVER", "Inalid Command: `{}`", cmd);
    return;
}

void ServerLayer::OnClientConnected()
{

    LOG_ERROR_TAG("TEMP", "Client Connected");
}

void ServerLayer::OnDataReceived(const aero::Buffer buf)
{
    std::string raw = std::format(R"({0})", buf.As<char>());
    LOG_WARN_TAG("TEMP", "Raw data recieved: {}", raw);
    LOG_WARN_TAG("TEMP", "Bytes recieved: {}", buf.size);
    LOG_WARN_TAG("TEMP", "Data recieved: {}", buf.As<char>());
    m_TCP->SendBuffer(buf);
}
