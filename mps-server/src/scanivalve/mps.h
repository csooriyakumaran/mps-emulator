#ifndef __SCANIVALVE_MPS_H_
#define __SCANIVALVE_MPS_H_

#include <chrono>
#include <queue>
#include <random>
#include <stdint.h>
#include <string>
#include <thread>

#include "aero/core/buffer.h"
#include "aero/core/threads.h"
#include "aero/networking/server.h"

#include "mps-data.h"

namespace mps
{

enum class Status
{
    READY = 0,
    SCAN,
    CALZ,
    SAVE,
    CAL,
    VAL,
    PGM,
    _COUNT
};

static std::string StatusStr[(size_t)Status::_COUNT] = {"READY", "SCAN", "CALZ", "SAVE",
                                                        "CAL",   "VAL",  "PGM"};

struct ScannerCfg
{
    int32_t fps           = 0;
    float framerate       = 850;
    float out_rate        = 10;
    int32_t unit_index    = 23;
    int32_t serial_number = 23;
    bool enudp = true;
    uint16_t udp_port     = 24;
    std::string udp_ip    = "127.0.0.1";
};

class Mps
{
public:
    Mps(const ScannerCfg& cfg, uint64_t id, std::shared_ptr<aero::networking::Server> svr);
    ~Mps();

    void Start();
    void Shutdown();
    void StartScan();
    void StopScan();

    void CalZ();
    std::string ParseCommands(std::string cmds);

    std::string GetStatus() { return StatusStr[(size_t)m_Status]; }
    Status GetStatus() const { return m_Status; }
    int32_t GetValveStatus() const { return 0; }

private:
    void ScanThreadFn();
    uint8_t NumAverages();
    float Sample();

private:
    std::string m_Name;
    aero::Buffer m_Data;
    std::queue<mps::BinaryPacket> m_FrameQueue;
    std::jthread m_ScanningThread;

    aero::ThreadPool m_ThreadPool;

    ScannerCfg m_cfg;
    Status m_Status    = Status::READY;

    bool m_Running     = false;
    bool m_Scanning    = false;
    bool m_Calibrating = false;

    uint64_t m_ClientID;

    uint32_t m_naverages = 1;

    std::mt19937 m_RandomGenerator;
    std::normal_distribution<float> m_NormalDistribution;
    // std::chrono::time_point<std::chrono::high_resolution_clock> m_StartScanTime;
    std::chrono::time_point<std::chrono::steady_clock> m_StartScanTime;
    uint32_t m_StartScanTimeS;
    uint32_t m_StartScanTimeNS;

    std::shared_ptr<aero::networking::Server> m_Server;
};

} // namespace mps

#endif // __SCANIVALVE_MPS_H_
