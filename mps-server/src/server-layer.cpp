#include "server-layer.h"

#include "aero/core/application.h"
#include "aero/core/log.h"
#include "aero/networking/utils.h"

#include "utils/string-utils.h"

//- Initialize members
ServerLayer::ServerLayer(uint32_t id, std::string ip, uint16_t port, bool enable_console)
    : m_DeviceID(id),
      m_BindIp(ip),
      m_Port(port),
      m_EnableConsole(enable_console)
{
}

ServerLayer::~ServerLayer() {}

void ServerLayer::OnAttach()
{

    //- Device

    mps::MpsConfig cfg; //  todo(Chris): make cfg load from yaml
    cfg.device_id       = (MpsDeviceID)m_DeviceID;
    cfg.serial_number   = aero::extract_last_octet(m_BindIp);
    cfg.telnet_format   = MPS_FMT_ASCII;
    cfg.ftp_udp_format  = MPS_FMT_BINARY;
    cfg.binary_format   = MPS_FMT_BINARY;
    cfg.scan_rate_hz    = 10;
    cfg.frames_per_scan = 0;
    cfg.units           = MPS_UNITS_PA;
    cfg.udp_enabled     = true;
    cfg.udp_target_ip   = "127.0.0.1";
    cfg.udp_target_port = 23u;
    cfg.ftp_enabled     = false;
    cfg.sim             = MPS_SIM_SIMULATED_64CH;

    m_Device = std::make_unique<mps::Mps>(
        cfg,
        [this](const aero::Buffer& frame) { this->OnScanFrame(frame); },
        [this](const aero::Buffer& frame) { this->OnLabviewFrame(frame); },
        [this]() { this->OnStatusChanged(); }
    );
    m_Device->Start();

    //- Set up the TCP / Telnet command/control server on specified port (default 23) and bind to specified ip (default
    //  INADDR_ANY)
    //  This will also handle UDP data streaming
    aero::networking::ServerInfo server_info;
    server_info.name        = "CONTROL-SERVER";
    server_info.workers     = 5;
    server_info.port        = m_Port;
    server_info.bind_ip     = m_BindIp;
    server_info.buffer_size = 1024;
    server_info.enable_udp  = true;

    //- Start the server and hook into the callbacks
    m_Server = std::make_unique<aero::networking::Server>(server_info);
    m_Server->SetClientConCallback([this](uint64_t id) { this->OnClientConnected(id); });
    m_Server->SetClientDisconCallback([this](uint64_t id) { this->OnClientDisconnected(id); });
    m_Server->SetDataRecvCallback([this](uint64_t id, aero::Buffer buf) { this->OnDataReceived(id, buf); });
    m_Server->Start();

    //- set up the TCP binary server on port 503 (to mirror real MPS devices)
    aero::networking::ServerInfo tcp_binary_server_info;
    tcp_binary_server_info.name        = "TCP-BINARY-SERVER";
    tcp_binary_server_info.workers     = 2;
    tcp_binary_server_info.port        = 503;
    tcp_binary_server_info.bind_ip     = m_BindIp;
    tcp_binary_server_info.buffer_size = 1024;
    tcp_binary_server_info.enable_udp  = false;

    m_TCPBinaryServer = std::make_unique<aero::networking::Server>(tcp_binary_server_info);
    m_TCPBinaryServer->SetClientConCallback([this](uint64_t id) { this->OnTCPBinaryClientConnected(id); });
    m_TCPBinaryServer->SetClientDisconCallback([this](uint64_t id) { this->OnTCPBinaryClientDisconnected(id); });
    m_TCPBinaryServer->SetDataRecvCallback([this](uint64_t id, aero::Buffer buf)
                                           { this->OnTCPBinaryDataReceived(id, buf); });
    m_TCPBinaryServer->Start();

    if (m_EnableConsole)
    {
        m_Console = std::make_unique<Console>();
        m_Console->SetMessageCallback([this](std::string_view msg) { this->OnConsoleInput(msg); });
        // mps::ScannerCfg cfg;
        // m_Scanners[0] = std::make_unique<mps::Mps>(cfg, 0, m_Server, m_TCPBinaryServer);
        // m_Scanners[0]->Start();
    }
}

void ServerLayer::OnDetach()
{
    if (m_Device)
        m_Device->Shutdown();

    m_Device = nullptr;

    if (m_Server)
        m_Server->Stop();

    m_Server = nullptr;

    if (m_TCPBinaryServer)
        m_TCPBinaryServer->Stop();

    m_TCPBinaryServer = nullptr;
}

void ServerLayer::OnUpdate()
{
    //- if the server shutsdown for some reason, restart it.
    if (!m_Server->IsRunning())
        m_Server->Start();

    if (!m_TCPBinaryServer->IsRunning())
        m_TCPBinaryServer->Start();

    // LOG_DEBUG_TAG("MPS-EMULATOR", "Running at {:8.2f} fps", aero::Application::Get().FrameRate());
}
// ---- D E V I C E - C A L L B A C K -------------------------------------

void ServerLayer::OnScanFrame(aero::Buffer buf)
{
    LOG_DEBUG_TAG("MPS-EMULATOR", "Scan frame of size {} recieved", buf.size);

    if (m_Device && m_Device->GetConfig().udp_enabled && m_Device->GetConfig().ftp_udp_format == MPS_FMT_BINARY)
    {
        std::string ip = m_Device->GetConfig().udp_target_ip;
        uint16_t port  = m_Device->GetConfig().udp_target_port;
        m_Server->StreamTo(ip, port, aero::Buffer::Copy(buf.data, buf.size)); // queues to send
    }

    if (m_TCPBinaryCleintConnected && m_TCPBinaryServer && m_Device->GetConfig().binary_format == MPS_FMT_BINARY)
    {
        m_TCPBinaryServer->SendBuffer(m_ActiveTCPBinaryClientId, aero::Buffer::Copy(buf.data, buf.size));
    }

    buf.Release();
}

void ServerLayer::OnLabviewFrame(aero::Buffer buf)
{
    if (m_TCPBinaryCleintConnected && m_TCPBinaryServer && m_Device->GetConfig().binary_format == MPS_FMT_LABVIEW)
    {
        // - swap byte order for MPS link only
        aero::SwapToNetworkOrderInPlace(buf);
        m_TCPBinaryServer->SendBuffer(m_ActiveTCPBinaryClientId, aero::Buffer::Copy(buf.data, buf.size));
    }

    buf.Release();

}

void ServerLayer::OnStatusChanged()
{

    LOG_INFO_TAG("MPS-EMULATOR", "Device state changed to {}", m_Device->GetStatusStr());

    if (m_Device->GetStatus() == MPS_STATUS_READY && m_Server && m_TCPCommandClientConnected &&
        m_ActiveTCPCommandClientId != 0)
    {
        m_Server->SendString(m_ActiveTCPCommandClientId, "\r\n>");
    }
}
// ---- C O N S O L E - C A L L B A C K ---------------------------------------

void ServerLayer::OnConsoleInput(std::string_view msg)
{

    if (msg[0] == '/' && msg.size() > 1)
    {
        std::string_view cmd(&msg[1], msg.size() - 1);
        OnCommand(0, cmd); // TODO(chris): Handle this separately since we dont want to sent this to a single client
        return;
    }

    if (m_Server)
        m_Server->SendStringToAll(std::string(msg) + "\r\n>");
}

// ---- S E R V E R - C A L L B A C K S ---------------------------------------

void ServerLayer::OnClientConnected(uint64_t id)
{
    if (id == m_ActiveTCPCommandClientId)
        return;

    if (m_TCPCommandClientConnected)
    {
        LOG_INFO_TAG("MPS-EMULATOR", "New connection request .. Kicking client id {}", m_ActiveTCPCommandClientId);
        m_Server->KickClient(m_ActiveTCPCommandClientId);
        m_Cmds.erase(m_ActiveTCPCommandClientId);
    }

    LOG_INFO_TAG("MPS-EMULATOR", "Connection esbablished with client id {}", id);
    m_ActiveTCPCommandClientId  = id;
    m_TCPCommandClientConnected = true;
    m_Server->SendString(id, "\r\n>");
}

void ServerLayer::OnClientDisconnected(uint64_t id)
{
    if (id == m_ActiveTCPCommandClientId)
    {
        LOG_INFO_TAG("MPS-EMULATOR", "Connection lost with client id {}", id);
        m_TCPCommandClientConnected = false;
        m_ActiveTCPCommandClientId  = 0;
        m_Cmds.erase(id);
    }
}

void ServerLayer::OnDataReceived(uint64_t id, aero::Buffer buf)
{

    std::string_view cmd(buf.As<char>(), buf.size);
    LOG_DEBUG_TAG("MPS-EMULATOR", "Data revieced from client on port 23: `{}`", cmd);

    // validate input
    if (!IsValidMsg(cmd))
    {
        LOG_WARN_TAG("MPS-EMULATOR", "`{}` is not a valid command", cmd);
        buf.Release();
        return;
    }

    //- handle command
    OnCommand(id, cmd);
    buf.Release();
    return;
}

// ----  T C P - S E R V E R --------------------------------------------------

void ServerLayer::OnTCPBinaryClientConnected(uint64_t id)
{
    if (id == m_ActiveTCPBinaryClientId)
        return;

    if (m_TCPBinaryCleintConnected && m_TCPBinaryServer)
    {
        LOG_INFO_TAG("MPS-EMULATOR", "Kicking binary client id {}", m_ActiveTCPBinaryClientId);
        m_TCPBinaryServer->KickClient(m_ActiveTCPBinaryClientId);
    }

    LOG_INFO_TAG("MPS-EMULATOR", "Connection esbablished with binary client id {}", id);
    m_ActiveTCPBinaryClientId  = id;
    m_TCPBinaryCleintConnected = true;
}

void ServerLayer::OnTCPBinaryClientDisconnected(uint64_t id)
{
    if (id == m_ActiveTCPBinaryClientId)
    {
        LOG_INFO_TAG("MPS-EMULATOR", "Connection lost with binary client id {}", id);
        m_TCPBinaryCleintConnected = false;
        m_ActiveTCPBinaryClientId  = 0;
    }
}

void ServerLayer::OnTCPBinaryDataReceived(uint64_t id, aero::Buffer buf)
{
    if (!m_Device)
    {
        buf.Release();
        return;
    }

    if (buf.size >= 1)
    {
        uint8_t val = *buf.As<uint8_t>();
        LOG_DEBUG_TAG("MPS-EMULATOR", "Data recieved on port 503: `{}`", val);
        if (val == 1)
            m_Device->StartScan();
        if (val == 0)
            m_Device->StopScan();
    }

    buf.Release();
}
// ---- P R O C E S S I N G ---------------------------------------------------

bool ServerLayer::IsValidMsg(const std::string_view& msg) { return true; }

void ServerLayer::OnCommand(uint64_t id, std::string_view cmd)
{

    if (m_Server && id != m_ActiveTCPCommandClientId && id != 0)
    {
        m_Server->SendString(id, "[Error] you do not have permission to control this device\r\n");
        return;
    }

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
            m_Server->SendString(id, "Shuting down server: Requires manually stopping from the server console\r\n");

        return aero::Application::Get().Shutdown();
    }

    if (tokens[0] == "restart" || tokens[0] == "RESTART" || tokens[0] == "reboot" || tokens[0] == "REBOOT")
    {
        if (m_EnableConsole && m_Server)
            m_Server->SendString(id, "Retarting down server: Requires manually stopping from the server console\r\n");
        return aero::Application::Get().Restart();
    }

    if (m_Device)
    {
        std::string response = m_Device->ParseCommands(tokens[0]);
        if (id == 0)
            std::cout << response << "\n";
        m_Server->SendString(id, response);
    }

    // if (m_Scanners.find(id) != m_Scanners.end())
    // {
    //     std::string response = m_Scanners[id]->ParseCommands(tokens[0]);
    //     m_Server->SendString(id, response);
    // }

    // LOG_ERROR_TAG("MPS-EMULATOR", "Inalid Command: `{}`", tokens[0]);
    // m_TCP->SendString(id, std::string("Invalid Command: ") + std::string(tokens[0]) + "\r\n>");

    return;
}
