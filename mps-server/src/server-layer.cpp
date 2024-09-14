#include "server-layer.h"

#include "aero/core/application.h"
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

    //-set up the TCP server
    aero::networking::ServerInfo server_info;
    server_info.name        = "SERVER";
    server_info.workers     = 5;
    server_info.port        = m_Port;
    server_info.buffer_size = 1024;

    //- Start the server and hook into the callbacks
    m_Server = std::make_shared<aero::networking::Server>(server_info);
    m_Server->SetClientConCallback([this](uint64_t id) { this->OnClientConnected(id); });
    m_Server->SetClientDisconCallback([this](uint64_t id) { this->OnClientDisconnected(id); });
    m_Server->SetDataRecvCallback([this](uint64_t id, const aero::Buffer buf) { this->OnDataReceived(id, buf); });
    m_Server->Start();

    if (m_EnableConsole)
    {
        m_Console = std::make_unique<Console>();
        m_Console->SetMessageCallback([this](std::string_view msg) { this->OnConsoleInput(msg); });
        mps::ScannerCfg cfg;
        m_Scanners[0] = std::make_unique<mps::Mps>(cfg, 0, m_Server);
        m_Scanners[0]->Start();
    }
}

void ServerLayer::OnDetach()
{
    for (auto& [id, mps] : m_Scanners)
        mps->Shutdown();

    m_Scanners.clear();

    if (m_Server)
        m_Server->Stop();

    m_Server = nullptr;
}

void ServerLayer::OnUpdate()
{
    //- if the server shutsdown for some reason, restart it.
    if (!m_Server->IsRunning())
        m_Server->Start();

    LOG_DEBUG_TAG("ServerLayer", "Running at {:8.2f} fps", aero::Application::Get().FrameRate());
}
// ---- C O N S O L E - C A L L B A C K ---------------------------------------

void ServerLayer::OnConsoleInput(std::string_view msg)
{

    if (msg[0] == '/' && msg.size() > 1)
    {
        std::string_view cmd(&msg[1], msg.size() - 1);
        OnCommand(
            0, cmd
        ); // TODO(chris): Handle this separately since we dont want to sent this to a single client
        return;
    }

    m_Server->SendStringToAll(std::string(msg) + "\r\n>");
}

// ---- S E R V E R - C A L L B A C K S ---------------------------------------

void ServerLayer::OnClientConnected(uint64_t id)
{
    LOG_INFO_TAG("APP", "Connection esbablished with client id {}", id);

    if (m_Scanners.find(id) != m_Scanners.end())
    {
        LOG_WARN_TAG("APP", "Already started a scanner from this connection!");
    }

    mps::ScannerCfg cfg;
    m_Scanners[id] = std::make_unique<mps::Mps>(cfg, id, m_Server);
    m_Scanners[id]->Start();
}

void ServerLayer::OnClientDisconnected(uint64_t id)
{
    LOG_INFO_TAG("APP", "Connection lost with client id {}", id);
    m_Cmds.erase(id);

    m_Scanners.erase(id);
}

void ServerLayer::OnDataReceived(uint64_t id, const aero::Buffer buf)
{

    std::string_view cmd(buf.As<char>(), buf.size);

    LOG_DEBUG_TAG("APP", "Data revieced from client `{}`", cmd);

    // validate input

    //- handle command
    OnCommand(id, cmd);
}

// ----  T C P - S E R V E R --------------------------------------------------

// ---- P R O C E S S I N G ---------------------------------------------------

bool ServerLayer::IsValidMsg(const std::string_view& msg) { return false; }

void ServerLayer::OnCommand(uint64_t id, std::string_view cmd)
{

    std::string& client_cmd = m_Cmds[id];
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

            if (!client_cmd.empty())
                client_cmd.pop_back();
            return;
        }
        case (0x1b): // <ESC> - TODO(Chris): this should be treated as a stop scan
        {
            client_cmd.clear();
            client_cmd += 0x1b;
            break;
        }
        default:
        {
            client_cmd += cmd;
            return;
        }
        }
    }
    else
    {
        client_cmd = cmd;
    }

    std::vector<std::string> tokens = utils::SplitString(client_cmd, "\r\n");
    client_cmd.clear();

    if (tokens[0][0] == 0x0d || tokens[0].empty()) // <CR>
        return m_Server->SendString(id, "\r>");

    // return m_TCP->SendString(id, "STOP\r\n>");

    if (tokens[0] == "shutdown" || tokens[0] == "SHUTDOWN")
    {
        if (m_EnableConsole)
            m_Server->SendString(
                id, "Shuting down server: Requires manually stopping from the server console\r\n"
            );

        return aero::Application::Get().Shutdown();
    }

    if (tokens[0] == "restart" || tokens[0] == "RESTART" || tokens[0] == "reboot" ||
        tokens[0] == "REBOOT")
    {
        if (m_EnableConsole)
            m_Server->SendString(
                id, "Retarting down server: Requires manually stopping from the server console\r\n"
            );
        return aero::Application::Get().Restart();
    }


    if (m_Scanners.find(id) != m_Scanners.end())
    {
        std::string response = m_Scanners[id]->ParseCommands(tokens[0]);
        m_Server->SendString(id, response);
    }

    // LOG_ERROR_TAG("APP", "Inalid Command: `{}`", tokens[0]);
    // m_TCP->SendString(id, std::string("Invalid Command: ") + std::string(tokens[0]) + "\r\n>");

    return;
}
