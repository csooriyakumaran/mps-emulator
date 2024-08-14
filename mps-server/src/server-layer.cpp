#include "server-layer.h"
#include <iostream>

ServerLayer::ServerLayer() {}

ServerLayer::~ServerLayer() {}

void ServerLayer::OnAttach()
{
    int num = 42;
    m_Console.AddTaggedMsg("TCP", "On Attach {} .. \n", num);
}

void ServerLayer::OnDetach() { std::cout << "[ TCP  ] On Detach .. \n"; }

void ServerLayer::OnUpdate() { /*std::cout << "[ TCP  ] On Update .. \n";*/ }
