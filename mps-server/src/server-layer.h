#ifndef _MPS_SERVER_LAYER_H_
#define _MPS_SERVER_LAYER_H_

#include <memory>
#include "aero/core/layer.h"

#include "console.h"

class ServerLayer : public aero::Layer
{
public:
    ServerLayer();
    ~ServerLayer();

    virtual void OnAttach() override;
    virtual void OnDetach() override;
    virtual void OnUpdate() override;

private:
    // ---- S E R V E R - C A L L B A C K S -----------------------------------
    /*void OnClientConnected(const aero::networking::ClientID& client);*/
    /*void OnClientDisconnected(const aero::networking::ClientID& client);*/
    /*void OnDataReceived(const aero::networking::ClientID& client, const aero::Buffer);*/

    // ---- S E R V E R - C A L L B A C K S -----------------------------------
private:
    Console m_Console;
};
#endif // _MPS_SERVER_LAYER_H_
