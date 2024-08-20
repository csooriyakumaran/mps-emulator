#include "server-layer.h"
#include <chrono>
#include <format>
#include <iostream>
#include <sstream>

#include "aero/core/application.h"
#include "aero/core/assert.h"
#include "aero/core/log.h"

#include "utils/string-utils.h"

//- Initialize members
ServerLayer::ServerLayer(uint16_t port, bool enable_console)
    : m_Port(port),
      m_EnableConsole(enable_console)
{
}

ServerLayer::~ServerLayer() {}

void ServerLayer::OnAttach()
{
    if (m_EnableConsole)
    {
        m_Console = std::make_unique<Console>();
        m_Console->SetMessageCallback([this](std::string_view msg) { this->OnConsoleInput(msg); });
    }

    //-set up the TCP server
    aero::networking::ServerInfo tcp_info;
    tcp_info.name        = "TCP-SERVER";
    tcp_info.workers     = 5;
    tcp_info.port        = m_Port;
    tcp_info.buffer_size = 1024;

    m_TCP                = std::make_unique<aero::networking::Server>(tcp_info);
    m_TCP->SetClientConCallback([this]() { this->OnClientConnected(); });
    m_TCP->SetDataRecvCallback([this](uint64_t id, const aero::Buffer buf)
                               { this->OnDataReceived(id, buf); });
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
    //- if the server shutsdown for some reason, restart it.
    if (!m_TCP->IsRunning())
        m_TCP->Start();

    /*if (!m_UDP->IsRunning())*/
    /*    m_UDP->Start();*/

    LOG_DEBUG_TAG("ServerLayer", "Running at {:8.2f} fps", aero::Application::Get().FrameRate());
}
// ---- C O N S O L E - C A L L B A C K ---------------------------------------

void ServerLayer::OnConsoleInput(std::string_view msg)
{

    if (msg[0] == '/' && msg.size() > 1)
    {
        std::string_view cmd(&msg[1], msg.size() - 1);
        /*OnCommand(cmd); // TODO(chris): Handle this separately since we dont want to sent this to a single client*/
        return;
    }

    m_TCP->SendStringToAll(std::string(msg) + "\r\n>");
}

// ---- S E R V E R - C A L L B A C K S ---------------------------------------

void ServerLayer::OnClientConnected() {}

void ServerLayer::OnClientDisconnected() {}

void ServerLayer::OnDataReceived(uint64_t id, const aero::Buffer buf)
{

    std::string_view cmd(buf.As<char>(), buf.size);

    LOG_DEBUG_TAG("SERVER", "Data revieced from client `{}`", cmd);

    // validate input

    //- handle command
    OnCommand(id, cmd);
}

// ----  T C P - S E R V E R --------------------------------------------------
/**/
/*void ServerLayer::SendMsg(std::string_view msg)*/
/*{*/
/*    if (msg.empty())*/
/*        return;*/
/**/
/*    //- echo message to server console*/
/*    m_TCP->SendString(std::string(msg));*/
/*}*/
/**/

// ---- U D P - S E R V E R ---------------------------------------------------

/*void ServerLayer::SendData(aero::Buffer buf)*/
/*{*/
/**/
/*    m_UDP->SendBuffer(aero::Buffer::Copy(buf));*/
/*}*/

// ---- P R O C E S S I N G ---------------------------------------------------

bool ServerLayer::IsValidMsg(const std::string_view& msg) { return false; }

void ServerLayer::OnCommand(uint64_t id, std::string_view cmd)
{

    if (cmd.size() == 1) // connected via ScanTel, need to build the command
    {
        switch (cmd[0])
        {
        case (0x0d): // <CR>
        {
            break;
        }
        case (0x08): // backspace
        {

            if (!m_CmdString.empty())
                m_CmdString.pop_back();
            return;
        }
        case (0x1b): // <ESC> - TODO(Chris): this should be treated as a stop scan
            break;
        default:
        {
            m_CmdString += cmd;
            return;
        }
        }
    }
    else
    {
        m_CmdString = cmd;
    }

    std::vector<std::string> tokens = utils::SplitString(m_CmdString, "\r\n");
    m_CmdString.clear();

    if (tokens[0][0] == 0x0d || tokens[0].empty()) // <CR>
        return m_TCP->SendString(id, "\r>");

    if (tokens[0][0] == 0x1b) // <ESC>
        return m_TCP->SendString(id, "STOP\r\n>");

    if (tokens[0] == "shutdown" || tokens[0] == "SHUTDOWN")
    {
        if (m_EnableConsole)
            m_TCP->SendString(
                id, "Shuting down server: Requires manually stopping from the server console\r\n"
            );
        return aero::Application::Get().Shutdown();
    }

    if (tokens[0] == "restart" || tokens[0] == "RESTART" || tokens[0] == "reboot" ||
        tokens[0] == "REBOOT")
    {
        if (m_EnableConsole)
            m_TCP->SendString(
                id, "Retarting down server: Requires manually stopping from the server console\r\n"
            );
        return aero::Application::Get().Restart();
    }

    if (tokens[0] == "ver" || tokens[0] == "VER" || tokens[0] == "version" ||
        tokens[0] == "VERSION")
        return m_TCP->SendString(id, "Aiolos (c) MPS Server Emulator v.2024.0\r\n>");

    if (tokens[0] == "status" || tokens[0] == "STATUS")
        return m_TCP->SendString(id, "STATUS: READY\r\n>");

    LOG_ERROR_TAG("SERVER", "Inalid Command: `{}`", tokens[0]);
    m_TCP->SendString(id, std::string("Invalid Command: ") + std::string(tokens[0]) + "\r\n>");

    return;
}
