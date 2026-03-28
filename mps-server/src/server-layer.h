#ifndef _MPS_SERVER_LAYER_H_
#define _MPS_SERVER_LAYER_H_

#include <map>
#include <memory>
#include <stdint.h>
#include <string>
#include <string_view>

#include "aero/core/layer.h"
#include "aero/networking/server.h"
#include "console.h"
#include "scanivalve/mps.h"

class ServerLayer : public aero::Layer
{
public:
    ServerLayer(uint32_t id, std::string ip, uint16_t port, bool enable_console = true);
    ~ServerLayer();

    virtual void OnAttach() override;
    virtual void OnDetach() override;
    virtual void OnUpdate() override;

private:
    void StartServers();
    void StopServers();

    // ---- D E V I C E - C A L L B A C K -------------------------------------
    void OnScanFrame(aero::Buffer buf);
    void OnLabviewFrame(aero::Buffer buf);
    void OnStatusChanged();

    // ---- C O N S O L E - C A L L B A C K -----------------------------------
    void OnConsoleInput(std::string_view msg);

    // ---- S E R V E R - C A L L B A C K S -----------------------------------
    void OnClientConnected(uint64_t id);
    void OnClientDisconnected(uint64_t id);
    void OnDataReceived(uint64_t id, aero::Buffer buf);

    void OnTCPBinaryClientConnected(uint64_t id);
    void OnTCPBinaryClientDisconnected(uint64_t id);
    void OnTCPBinaryDataReceived(uint64_t id, aero::Buffer buf);

    // ----  T C P - S E R V E R ----------------------------------------------
    void SendMsg(std::string_view msg);

    // ----  U P D - S E R V E R ----------------------------------------------
    void SendDataGram(aero::Buffer buff);

    // ---- P R O C E S S I N G -----------------------------------------------
    bool IsValidMsg(const std::string_view& msg);
    void OnCommand(uint64_t id, std::string_view cmd);

private:
    uint32_t m_DeviceID;
    uint16_t m_Port;
    std::string m_BindIp;

    bool m_TCPCommandClientConnected    = false;
    bool m_TCPBinaryCleintConnected     = false;
    uint64_t m_ActiveTCPCommandClientId = 0;
    uint64_t m_ActiveTCPBinaryClientId  = 0;

    bool m_EnableConsole               = true;
    std::unique_ptr<Console> m_Console = nullptr;
    std::unique_ptr<mps::Mps> m_Device = nullptr;

    std::unique_ptr<aero::networking::Server> m_Server          = nullptr;
    std::unique_ptr<aero::networking::Server> m_TCPBinaryServer = nullptr;

    // std::map<uint64_t, std::unique_ptr<mps::Mps>> m_Scanners;

    std::map<uint64_t, std::string> m_Cmds;

    // std::queue<Packets> m_ScanDataQueue;
};
#endif // _MPS_SERVER_LAYER_H_
