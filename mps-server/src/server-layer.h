#ifndef _MPS_SERVER_LAYER_H_
#define _MPS_SERVER_LAYER_H_

#include <map>
#include <memory>
#include <queue>
#include <stdint.h>
#include <string>
#include <string_view>
#include <vector>

#include "aero/core/layer.h"
#include "aero/networking/server.h"
#include "console.h"

class ServerLayer : public aero::Layer
{
public:
    ServerLayer(uint16_t port);
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
    void OnClientConnected();
    void OnClientDisconnected();
    void OnDataReceived(const aero::Buffer buf);

    // ----  T C P - S E R V E R ----------------------------------------------
    void SendMsg(std::string_view msg);

    // ----  U P D - S E R V E R ----------------------------------------------
    void SendDataGram(aero::Buffer buff);

    // ---- P R O C E S S I N G -----------------------------------------------
    bool IsValidMsg(const std::string_view& msg);
    void OnCommand(std::string_view cmd);

private:
    uint16_t m_Port = 0u;
    std::unique_ptr<aero::networking::Server> m_TCP;
    /*std::unique_ptr<aero::networking::Server> m_UDP;*/

    //- user input console for server application
    std::unique_ptr<Console> m_Console;


    // std::queue<Packets> m_ScanDataQueue;
};
#endif // _MPS_SERVER_LAYER_H_
