#ifndef __SCANIVALVE_MPS_H_
#define __SCANIVALVE_MPS_H_

#include <vector>
#include <thread>
#include <fucntional>

#include "aero/core/log.h"
#include "aero/core/buffer.h"
#include "scanivavle/mps-data.h"

namespace mps
{

enum class Status
{
    READY = 0, SCAN, CALZ, SAVE, CAL, VAL, PGM, _COUNT
};

std::string StatusStr[(size_t)Status::_COUNT] = {
    "READY", "SCAN", "CALZ", "SAVE", "CAL", "VAL", "PGM"
};

struct ScannerCfg
{

};

class Mps
{
public:
    Mps(const ScannerCfg& cfg);
    ~Mps();

    void Start();
    void Shutdown();
    void StartScan() { m_Scanning = true; }
    void StopScan() { m_Scanning = false; }

    std::string ParseCommands(std::vector<std::string> cmds);

    std::string GetStatus() { return StatusStr[(size_t)m_Status]; }
    Status GetStatus() const { return m_Status; }

    const aero::Buffer& sample() { return m_CurrentValueTable; } 
private:
    std::function<void()> ScanThreadFn();

private:

    ScannerCfg m_cfg;
    Status m_Status = Status::READY;

    bool m_Running = False;
    bool m_Scanning = False;
    aero::Buffer m_CurrentValueTable;
    std::thread  m_ScanningThread;

};



} // namespace mps

#endif // __SCANIVALVE_MPS_H_

