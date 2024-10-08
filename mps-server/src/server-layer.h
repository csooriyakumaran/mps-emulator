#ifndef _MPS_SERVER_LAYER_H_
#define _MPS_SERVER_LAYER_H_

#include <map>
#include <memory>
#include <stdint.h>
#include <string>
#include <string_view>

#include "aero/core/layer.h"
#include "aero/networking/server.h"
#include "scanivalve/mps.h"
#include "console.h"

class ServerLayer : public aero::Layer
{
public:
    ServerLayer(uint16_t port, bool enable_console = true);
    ~ServerLayer();

    virtual void OnAttach() override;
    virtual void OnDetach() override;
    virtual void OnUpdate() override;

private:
    void StartServers();
    void StopServers();

    // ---- C O N S O L E - C A L L B A C K -----------------------------------
    void OnConsoleInput(std::string_view msg);

    // ---- S E R V E R - C A L L B A C K S -----------------------------------
    void OnClientConnected(uint64_t id);
    void OnClientDisconnected(uint64_t id);
    void OnDataReceived(uint64_t id, aero::Buffer buf);

    // ----  T C P - S E R V E R ----------------------------------------------
    void SendMsg(std::string_view msg);

    // ----  U P D - S E R V E R ----------------------------------------------
    void SendDataGram(aero::Buffer buff);

    // ---- P R O C E S S I N G -----------------------------------------------
    bool IsValidMsg(const std::string_view& msg);
    void OnCommand(uint64_t id, std::string_view cmd);

private:
    uint16_t m_Port = 0u;

    bool m_EnableConsole = true;
    std::unique_ptr<Console> m_Console = nullptr;

    std::shared_ptr<aero::networking::Server> m_Server = nullptr;
    std::map<uint64_t, std::unique_ptr<mps::Mps>> m_Scanners;

    std::map<uint64_t, std::string> m_Cmds;

    //- user input console for server application


    // std::queue<Packets> m_ScanDataQueue;
};
#endif // _MPS_SERVER_LAYER_H_
