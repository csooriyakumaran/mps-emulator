#include "scanivalve/mps.h"

#include <chrono>
#include <cstring>
#include <iostream>
#include <sstream>

#include "aero/core/log.h"
#include "scanivalve/mps-config.h"
#include "scanivalve/mps-data.h"
#include "utils/string-utils.h"

mps::Mps::Mps(const mps::ScannerCfg& cfg, uint64_t id)
    : m_cfg(cfg), m_ThreadPool(2), m_ClientID(id)
{
}

mps::Mps::~Mps() { Shutdown(); }

void mps::Mps::Start()
{
    m_Running        = true;
    m_ScanningThread = std::jthread([this]() { this->ScanThreadFn(); });
    m_ThreadPool.Enqueue( [this]() { this->ScanThreadFn(); }, "Scanning Thread");
    m_ThreadPool.Enqueue( [this]() { this->TransferThreadFn(); }, "Transfer Thread");
}


void mps::Mps::Shutdown()
{

    StopScan();
    m_Running = false;

    if (m_ScanningThread.joinable())
        m_ScanningThread.join();
}

void mps::Mps::CalZ()
{
    m_Calibrating = true;
    m_Status = mps::Status::CALZ;
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

    data->type              = 0x0A; // BinaryPacket
    data->size              = sizeof(BinaryPacket);
    data->serial_number     = m_cfg.serial_number;
    data->framerate         = m_cfg.framerate;
    data->valve_status      = GetValveStatus();
    data->unit_index        = m_cfg.unit_index;
    data->unit_conversion   = mps::ScanUnitConversion[m_cfg.unit_index];
    // auto tp          = std::chrono::high_resolution_clock::now();
    auto tp                      = std::chrono::steady_clock::now();
    auto tp_s                    = std::chrono::time_point_cast<std::chrono::seconds>(tp);
    auto tp_ns                   = tp - tp_s;

    m_StartScanTime              = tp;
    m_StartScanTimeS             = static_cast<uint32_t>(tp_s.time_since_epoch().count());
    m_StartScanTimeS             = static_cast<uint32_t>(tp_ns.count());
    data->ptp_scan_start_time_s  = m_StartScanTimeS;
    data->ptp_scan_start_time_ns = m_StartScanTimeNS;

    m_naverages                  = (int)(m_cfg.framerate / m_cfg.out_rate);
    m_Scanning                   = true;
    m_Status                     = mps::Status::SCAN;
    // LOG_INFO_TAG("MPS", "Scan started at {}", tp);
}

void mps::Mps::StopScan()
{
    m_Scanning = false;
    m_Status   = mps::Status::READY;

    while (!m_FrameQueue.empty())
        m_FrameQueue.pop();

}

void mps::Mps::TransferThreadFn(std::shared_ptr<aero::networking::Server> svr )
{
    while (m_Running)
    {
        while (!m_FrameQueue.empty())
        {
            svr->SendBuffer(uint64_t id, Buffer buff);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void mps::Mps::ScanThreadFn()
{
    while (m_Running)
    {
        if (m_Calibrating)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            m_drift = 0;
            m_Calibrating = false;
            m_Status = mps::Status::READY;
        }

        auto frame_start = m_StartScanTime;
        while (m_Scanning)
        {
            auto frame_end = frame_start + std::chrono::nanoseconds((int)(1e9 / m_cfg.out_rate));

            auto dt        = frame_start - m_StartScanTime;
            auto dt_s      = std::chrono::duration_cast<std::chrono::seconds>(dt);
            auto dt_ns     = dt - dt_s;

            LOG_DEBUG_TAG(
                "MPS", "Frame start time: {:.8f} s ({} Hz)", dt_s.count() + dt_ns.count() / 1e9,
                m_cfg.out_rate
            );
            float p[64];
            memset(p, 0, 64 * sizeof(float));

            auto subframe_start = frame_start;
            for (size_t i = 0; i < (size_t)m_naverages; ++i)
            {
                auto subframe_end =
                    subframe_start + std::chrono::nanoseconds((int)(1e9 / m_cfg.framerate));

                for (int j = 0; j < 64; ++j)
                    p[j] += Sample() / m_naverages;

                auto sdt    = subframe_start - frame_start;
                auto sdt_s  = std::chrono::duration_cast<std::chrono::seconds>(sdt);
                auto sdt_ns = sdt - sdt_s;
                LOG_DEBUG_TAG(
                    "MPS", "SubFrame {:04d}: {:.8f} s ({} Hz) P[0] = {:.4f}", i,
                    sdt_s.count() + sdt_ns.count() / 1e9, m_cfg.framerate, p[0]
                );

                std::this_thread::sleep_until(subframe_end);
                subframe_start = subframe_end;
            }

            auto end_dt    = frame_end - m_StartScanTime;
            auto end_dt_s  = std::chrono::duration_cast<std::chrono::seconds>(end_dt);
            auto end_dt_ns = end_dt - end_dt_s;

            LOG_DEBUG_TAG(
                "MPS", "Frame end time: {:.8f} s ({} Hz) p[0] = {} Pa",
                end_dt_s.count() + end_dt_ns.count() / 1e9, m_cfg.out_rate, p[0]
            );

            mps::BinaryPacket* data = m_Data.As<mps::BinaryPacket>();
            data->frame += 1;

            std::memcpy(&p, &data->pressure, 64 * sizeof(float));
            data->frame_time_s  = static_cast<uint32_t>(dt_s.count());
            data->frame_time_ns = static_cast<uint32_t>(dt_ns.count());

            m_FrameQueue.push(*data);
            

            if ( m_cfg.fps && (m_cfg.fps == data->frame) )
                this->StopScan();



            std::this_thread::sleep_until(frame_end);
            frame_start = frame_end;

        }

        m_drift += 0.01f;

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
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

    std::vector<std::string> tokens = utils::SplitString(cmd, " \r\n");

    if (tokens[0] == "VER" || tokens[0] == "ver" || tokens[0] == "VERSION" ||
        tokens[0] == "version")
        return "Aiolos (c) MPS Server Emulator v.2024.0\r\n>";

    if (tokens[0] == "STATUS" || tokens[0] == "status")
        return std::string("STATUS: ") + this->GetStatus() + "\r\n>";

    if (tokens[0] == "CALZ" || tokens[0] == "calz")
    {
        this->CalZ();
        return "\r\n>";
    }

    if (tokens[0] == "SCAN" || tokens[0] == "scan")
    {
        this->StartScan();
        return "Scan started\r\n";
    }

    if (tokens[0] == "STOP" || tokens[0] == "stop" || tokens[0][0] == 0x1b) // <ESC>
    {
        this->StopScan();
        return "\r\n>";
    }

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

        if (tokens[1] == "FPS" || tokens[1] == "fps")
        {

            // [0] [1]   [2]
            // SET FPS <fps> 
            if (tokens.size() < 3)
                return "Invalid Command: `SET FPS` requires 1 argument\r\n>";

            std::sscanf(tokens[2].c_str(), "%d", &m_cfg.fps);
            LOG_INFO_TAG("MPS", "Frames Per Scan: {}", m_cfg.fps);
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
    return "Invalid Command: `" + cmd + "`\r\n>";
}
