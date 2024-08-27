#include "scanivalve/mps.h"

#include <chrono>
#include <cstring>
#include <iostream>

#include "scanivalve/mps-config.h"
#include "scanivalve/mps-data.h"

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

void mps::Mps::StartScan()
{
    std::random_device rd{};
    std::mt19937 gen{rd()};
    m_NormalDistribution = std::normal_distribution<float>(m_drift, 0.35f);

    //- get start time and conver to seconds/nanosecons
    auto tp          = std::chrono::high_resolution_clock::now();
    auto tp_s        = std::chrono::time_point_cast<std::chrono::seconds>(tp);
    auto tp_ns       = tp - tp_s;

    m_StartScanTime = tp;
    m_StartScanTimeS = static_cast<uint32_t>(tp_s.time_since_epoch().count());
    m_StartScanTimeS = static_cast<uint32_t>(tp_ns.count());
    m_Scanning = true;
}

void mps::Mps::Shutdown()
{

    m_Running = false;

    if (m_ScanningThread.joinable())
        m_ScanningThread.join();
}

void mps::Mps::ScanThreadFn()
{
    while (m_Running)
    {
        while (m_Scanning)
        {
            auto frame_start = std::chrono::high_resolution_clock::now();

            auto tp             = std::chrono::high_resolution_clock::now();
            auto dt             = tp - m_StartScanTime;
            auto dt_s           = std::chrono::duration_cast<std::chrono::seconds>(dt);
            auto dt_ns          = dt - dt_s;

            mps::BinaryPacket data;
            memset(&data, 0, sizeof(data));
            data.type = 0x0A; // BinaryPacket
            data.size = sizeof(BinaryPacket);
            data.frame += 1;
            data.serial_number            = m_cfg.serial_number;
            data.framerate                = m_cfg.framerate;
            data.valve_status             = GetValveStatus();
            data.unit_index               = m_cfg.unit_index;
            data.unit_conversion          = mps::ScanUnitConversion[m_cfg.unit_index];
            data.ptp_scan_start_time_s    = m_StartScanTimeS;
            data.ptp_scan_start_time_ns   = m_StartScanTimeNS;
            data.external_trigger_time_us = 0;

            float p[64];
            memset(p, 0, 64 * sizeof(float));

            for (int i = 0; i < m_naverages; ++i)
            {
                for (int j = 0; j < 64; ++j)
                    p[j] += Sample();

                /*std::this_thread::sleep_for(std::chrono::milliseconds((int)(1000 / m_cfg.framerate)));*/
            }
            for (int j = 0; j < 64; ++j)
                p[j] /= m_naverages;
            std::memcpy(&p, &data.pressure, 64 * sizeof(float));


            data.frame_time_s   = static_cast<uint32_t>(dt_s.count());
            data.frame_time_ns  = static_cast<uint32_t>(dt_ns.count());
            std::cout << "Frame Time: " << dt_s.count() + dt_ns.count() / 1e9 << '\n';
            m_CurrentValueTable = aero::Buffer::Copy(&data, sizeof(data));

            auto frame_end = std::chrono::high_resolution_clock::now();
            auto frame_time =  std::chrono::duration_cast<std::chrono::milliseconds>(frame_end - frame_start).count();

            auto wait_time = (int)(1000/m_cfg.out_rate) - frame_time;
            std::this_thread::sleep_for(std::chrono::milliseconds(wait_time));
        }

        m_drift += 0.0001f;

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

float mps::Mps::Sample()
{
    return [this]() { return this->m_NormalDistribution(this->m_RandomGenerator); }();
}
