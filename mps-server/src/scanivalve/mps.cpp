#include "scanivalve/mps.h"

#include <chrono>
#include <cstring>
#include <iostream>

#include "scanivalve/mps-config.h"
#include "scanivalve/mps-data.h"
#include "aero/core/log.h"
#include "utils/string-utils.h"

mps::Mps::Mps(const mps::ScannerCfg& cfg)
    : m_cfg(cfg)
{
}

mps::Mps::~Mps() { Shutdown(); }

void mps::Mps::Start()
{
    m_Running        = true;
    m_ScanningThread = std::jthread([this]() { this->ScanThreadFn(); });
}

void mps::Mps::Shutdown()
{

    StopScan();
    m_Running = false;

    if (m_ScanningThread.joinable())
        m_ScanningThread.join();
}

void mps::Mps::StartScan()
{
    std::random_device rd{};
    std::mt19937 gen{rd()};
    m_NormalDistribution = std::normal_distribution<float>(m_drift, 0.35f);

    //- get start time and conver to seconds/nanosecons

    m_Data.Allocate(sizeof(BinaryPacket));
    m_Data.Zero();

    mps::BinaryPacket* data = m_Data.As<mps::BinaryPacket>();

    data->type = 0x0A; // BinaryPacket
    data->size = sizeof(BinaryPacket);
    data->serial_number            = m_cfg.serial_number;
    data->framerate                = m_cfg.framerate;
    data->valve_status             = GetValveStatus();
    data->unit_index               = m_cfg.unit_index;
    data->unit_conversion          = mps::ScanUnitConversion[m_cfg.unit_index];
    // auto tp          = std::chrono::high_resolution_clock::now();
    auto tp          = std::chrono::steady_clock::now();
    auto tp_s        = std::chrono::time_point_cast<std::chrono::seconds>(tp);
    auto tp_ns       = tp - tp_s;


    m_StartScanTime  = tp;
    m_StartScanTimeS = static_cast<uint32_t>(tp_s.time_since_epoch().count());
    m_StartScanTimeS = static_cast<uint32_t>(tp_ns.count());
    data->ptp_scan_start_time_s    = m_StartScanTimeS;
    data->ptp_scan_start_time_ns   = m_StartScanTimeNS;

    m_Scanning       = true;
    // LOG_INFO_TAG("MPS", "Scan started at {}", tp);
}

void mps::Mps::StopScan() { m_Scanning = false; }

void mps::Mps::ScanThreadFn()
{
    while (m_Running)
    {
        auto frame_start = m_StartScanTime;
        while (m_Scanning)
        {
            //auto frame_start = std::chrono::steady_clock::now();
            auto frame_end   = frame_start + std::chrono::nanoseconds((int)(1e9/ m_cfg.framerate));

            auto dt          = frame_start - m_StartScanTime;
            auto dt_s        = std::chrono::duration_cast<std::chrono::seconds>(dt);
            auto dt_ns       = dt - dt_s;

            mps::BinaryPacket* data = m_Data.As<mps::BinaryPacket>();
            data->frame += 1;
            float p[64];
            memset(p, 0, 64 * sizeof(float));

            for (int i = 0; i < m_naverages; ++i)
            {
                for (int j = 0; j < 64; ++j)
                    p[j] += Sample();

                /*std::this_thread::sleep_for(std::chrono::milliseconds((int)(1000 /
                 * m_cfg.framerate)));*/
            }
            for (int j = 0; j < 64; ++j)
                p[j] /= m_naverages;

            std::memcpy(&p, &data->pressure, 64 * sizeof(float));

            data->frame_time_s  = static_cast<uint32_t>(dt_s.count());
            data->frame_time_ns = static_cast<uint32_t>(dt_ns.count());
            LOG_INFO_TAG("MPS", "Frame Time: {:.8f} s ({} Hz)", dt_s.count() + dt_ns.count() / 1e9 , m_cfg.framerate);

            std::this_thread::sleep_until(frame_end);
            frame_start = frame_end;
        }

        m_drift += 0.0001f;

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

float mps::Mps::Sample()
{
    return [this]() { return this->m_NormalDistribution(this->m_RandomGenerator); }();
}

/*
Commands can come from TCP clients, TelNet clients, or a .cfg file. 
These should be first split into separate commands (i.e., by \r\n) before
passing to this function. 

This function will then further divide each command into tokens and parse them 
according the the requested action. 


*/
std::string mps::Mps::ParseCommands(std::string cmd)
{

    std::vector<std::string> tokens = utils::SplitString(cmd, " ");

    if (tokens[0] == "SET" || tokens[0] == "set")
    {
        if (tokens.size() < 2)
            return "Invalid Command: `SET` requires arguments\r\n>";

        if (tokens[1] == "RATE" || tokens[1] == "rate")
        {
            // [0] [1]   [2]    [3] 
            // SET RATE <RATE> [<OUT RATE>]
            if (tokens.size() < 3)
                return "Invalid Command: `SET RATE` requires at least 1 arguments\r\n>";
            
            if (tokens.size() >= 3)
                std::sscanf(tokens[2].c_str(), "%f", &m_cfg.framerate);

            if (tokens.size() >= 4)
                std::sscanf(tokens[3].c_str(), "%f", &m_cfg.out_rate);

            LOG_INFO_TAG("MPS", "Frame Rate: {}, Out Rate: {}", m_cfg.framerate, m_cfg.out_rate);

            return cmd + "\r\n>";
        }

    }



    if (tokens[0] == "LIST" || tokens[0] == "list")
    {
        if (tokens.size() < 2)
            return "Invalid Command: `LIST` requires arguments\r\n>";

        if (tokens[1] == "S" || tokens[1] == "s")
        {
            // build a string stream response. TBD

            return cmd + "\r\n>";
        }

    }

    LOG_ERROR_TAG("MPS", "Inalid Command: `{}`", tokens[0]);
    return "Invalid Command: `" + cmd + "`"; 

}

