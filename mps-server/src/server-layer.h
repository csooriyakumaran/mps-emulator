#ifndef _MPS_SERVER_LAYER_H_
#define _MPS_SERVER_LAYER_H_

#include <map>
#include <memory>
#include <queue>
#include <stdint.h>
#include <string_view>

#include "aero/core/layer.h"
#include "aero/networking/server.h"
#include "console.h"

/*#include "console.h"*/

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

    // ---- S E R V E R - C A L L B A C K S -----------------------------------
    void OnClientConnected(const ClientID& client);
    void OnClientDisconnected(const ClientID& client);
    void OnDataReceived(const ClientID& client, const aero::Buffer);

    // ----  T C P - S E R V E R ----------------------------------------------
    void SendMsg(std::string_view msg);

    // ----  U P D - S E R V E R ----------------------------------------------
    void SendDataGram(aero::Buffer buff, ClientID id);

    // ---- P R O C E S S I N G -----------------------------------------------
    void OnCommand(std::string_view cmd);

private:
    uint16_t m_Port = 0u;
    std::unique_ptr<aero::networking::Server> m_TCP;
    std::unique_ptr<aero::networking::Server> m_UDP;
    std::unique_ptr<Console> m_Console;

    std::map<ClientID, aero::networking::ClientInfo> m_ConnectedClients;

    // std::queue<Packets> m_ScanDataQueue;
};
#endif // _MPS_SERVER_LAYER_H_
