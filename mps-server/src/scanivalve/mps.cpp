#include "scanivalve/mps.h"


mps::Mps::Mps(const mps::ScannerCfg& cfg)
    : m_cfg(cfg)
{
}

mps::Mps::Start()
{
    m_Running        = true;
    m_ScanningThread = std::jthread([this](){ this->ScanThreadFn(); });
}

mps::Mps::Shutdown()
{

    m_Running = false;

    if (m_ScanningThread.joinable())
        m_ScanningThread.join();
}
