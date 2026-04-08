#include "scanivalve/mps.h"

#include <chrono>
#include <cstring>
#include <sstream>
#include <cmath>
#include <cstdio>
#include <string_view>

#include "aero/core/log.h"

#include "utils/string-utils.h"
#include "version.h"

#include "scanivalve/mps-protocol.h"


mps::Mps::Mps(const MpsConfig& cfg, FrameCallback on_frame, FrameCallback on_labview, StatusCallback on_status_changed)
    : m_cfg(cfg),
      m_ThreadPool(2),
      m_Name(std::format("Virtual-MPS-{}-SN{}", (uint32_t)cfg.device_id, cfg.serial_number)),
      m_OnFrame(on_frame),
      m_OnLabviewFrame(on_labview),
      m_OnStatusChanged(on_status_changed)
{
    m_NormalDistribution = std::normal_distribution<double>(0.0, 0.0005 * static_cast<double>(MPS_MAX_ADC_COUNTS) );
}

mps::Mps::~Mps() { Shutdown(); }

void mps::Mps::Start()
{
    m_Running = true;
    m_ThreadPool.Enqueue([this]() { this->ScanThreadFn(); }, "Scanning Thread");
    LOG_INFO_TAG(m_Name, "Starting Virtual Scanner: Emulating firmware v{}", MPS_FIRMWARE_VERSION_STRING);
}

void mps::Mps::Shutdown()
{

    StopScan();
    // - stop the canning thread
    m_Running = false;
    
    m_ThreadPool.StopAll();
}

void mps::Mps::CalZ()
{
    m_Calibrating = true;
    m_Status      = MPS_STATUS_CALZ;

    if (m_OnStatusChanged)
        m_OnStatusChanged();
}

void mps::Mps::UpdateScanAverages()
{
    if (m_cfg.scan_rate_hz >= 500)
        m_nSubFrameAverages = 1;
    else
        m_nSubFrameAverages = static_cast<uint32_t>(std::ceil(500 / m_cfg.scan_rate_hz));
}

void mps::Mps::ConfigureScanLayout()
{

    const mps::MpsConfig& cfg = GetConfig();

    // - Labview format requested on TCP Binary Server
    m_UseLabview = (cfg.binary_format == MPS_FMT_LABVIEW);

    // - Binary format request on either TCP Binary Server, UDP server, or FTP server
    m_UseBinary  = (cfg.binary_format == MPS_FMT_BINARY) || ( (cfg.ftp_udp_format == MPS_FMT_BINARY) && (cfg.udp_enabled || cfg.ftp_enabled));

    // - transmit data in raw ADC Counts
    m_UseRaw     = (cfg.units == MPS_UNITS_RAW);

    // - Simulate the legacy MPS4264 Gen 1 format with zero padding (bit6 of SIM)
    m_ZeroPad    = (cfg.sim & MPS_SIM_SIMULATED_64CH) != 0;

    // - reset the packet info struct pointer
    m_PacketInfo = nullptr;
    m_PacketType = MPS_PKT_UNDEFINED;

    // - determine the number of physical channels (i.e., what we use to generate the data)
    switch (cfg.device_id)
    {
        case MPS_4264:
            m_PhysicalPressureChannelCount = 64;
            m_PhysicalTemperatureChannelCount = 8;
            break;
        case MPS_4232:
            m_PhysicalPressureChannelCount = 32;
            m_PhysicalTemperatureChannelCount = 4;
            break;
        case MPS_4216:
            m_PhysicalPressureChannelCount = 16;
            m_PhysicalTemperatureChannelCount = 4;
            break;
    }


    // - default assume packet size matches physical channels
    m_PacketPressureChannelCount = m_PhysicalPressureChannelCount;
    m_PacketTemperatureChannelCount = m_PhysicalTemperatureChannelCount;

    // - if we are zero-padding (SIM bit6) then we need to update the packet
    if (m_ZeroPad)
    {
        m_PacketPressureChannelCount = 64;
        m_PacketTemperatureChannelCount = 8;
    }

    // - if we need binary packets for TCP or UDP/FTP, determine the type
    if (m_UseBinary)
    {
        if (m_ZeroPad)
        {
            m_PacketType = MPS_PKT_LEGACY_TYPE;
        } else {
            switch (cfg.device_id)
            {
                case MPS_4264:
                    m_PacketType = m_UseRaw ? MPS_PKT_64_RAW_TYPE : MPS_PKT_64_TYPE;
                    break;
                case MPS_4232:
                    m_PacketType = m_UseRaw ? MPS_PKT_32_RAW_TYPE : MPS_PKT_32_TYPE;
                    break;
                case MPS_4216:
                    m_PacketType = m_UseRaw ? MPS_PKT_16_RAW_TYPE : MPS_PKT_16_TYPE;
            }
        }

        m_PacketInfo = ::mps_get_binary_packet_info_by_type(m_PacketType);

    }

    LOG_INFO_TAG(
        m_Name,
        "Format F {}, B {}, SIM 0x{:02X}, Units {}, Type 0x{:02X} ({} bytes)",
        ::mps_format_to_char(cfg.ftp_udp_format),
        ::mps_format_to_char(cfg.binary_format),
        (uint8_t)cfg.sim,
        ::mps_units_to_string(cfg.units),
        (uint8_t)m_PacketType,
        ::mps_packet_size_from_type(m_PacketType) ? ::mps_packet_size_from_type(m_PacketType) : ::mps_labview_packet_size(m_PacketPressureChannelCount)
    );




}

std::string_view mps::Mps::GetStatusStr() const
{
    const char* s = ::mps_status_to_string(m_Status);
    return s ? std::string_view{s} : std::string_view {};
}

bool mps::Mps::SetDataFormat(char type, char fmt)
{
    if (type == 'T' || type == 't')
    {
        // Telnet: allow A, F, C (ASCII, VT100, CSV), case-insensitive
        if (!(fmt == 'A' || fmt == 'a' || fmt == 'F' || fmt == 'f' || fmt == 'C' || fmt == 'c'))
            return false;
        return (bool)::mps_format_from_char(fmt, &m_cfg.telnet_format);
    }

    if (type == 'F' || type == 'f')
    {
        // FTP/UDP: allow A, B, C (ASCII, BINARY, CSV), case-insensitive
        if (!(fmt == 'A' || fmt == 'a' || fmt == 'B' || fmt == 'b' || fmt == 'C' || fmt == 'c'))
            return false;
        return (bool)::mps_format_from_char(fmt, &m_cfg.ftp_udp_format);
    }

    if (type == 'B' || type == 'b')
    {
        // Binary (TCP 503): allow B or L (Binary or LabVIEW), case-insensitive
        if (!(fmt == 'B' || fmt == 'b' || fmt == 'L' || fmt == 'l'))
            return false;
        return (bool)::mps_format_from_char(fmt, &m_cfg.binary_format);
    }

    return false;
}

std::string mps::Mps::BuildDataFormatResponse()
{
    std::stringstream ss;
    ss << " T " << ::mps_format_to_char(m_cfg.telnet_format) << ",";
    ss << " F " << ::mps_format_to_char(m_cfg.ftp_udp_format) << ",";
    ss << " B " << ::mps_format_to_char(m_cfg.binary_format);
    return ss.str();
}

void mps::Mps::StartScan()
{
    if (m_Scanning)
        return;

    // - re-seed randomom number generator to ensur repeatable data for testing
    std::size_t seed = std::hash<std::string>{}(m_Name);
    m_RandomGenerator.seed(static_cast<uint32_t>(seed));

    m_ScanStartTimePTP = std::chrono::system_clock::now();
    m_ScanStartTime    = std::chrono::steady_clock::now();

    auto secs_since_epoch = std::chrono::time_point_cast<std::chrono::seconds>(m_ScanStartTimePTP);
    auto ns_since_epoch   = std::chrono::time_point_cast<std::chrono::nanoseconds>(m_ScanStartTimePTP);
    auto ms_since_epoch   = std::chrono::duration_cast<std::chrono::milliseconds>(m_ScanStartTimePTP.time_since_epoch());
    auto ms_fraction      = ms_since_epoch % 1000;

    m_ScanStartTimePTPSeconds     = static_cast<uint32_t>(secs_since_epoch.time_since_epoch().count());
    m_ScanStartTimePTPNanoseconds = static_cast<uint32_t>((ns_since_epoch - secs_since_epoch).count());

    ConfigureScanLayout();
    UpdateScanAverages();


    m_Scanning  = true;
    m_Status    = MPS_STATUS_SCAN;

   if (m_OnStatusChanged)
        m_OnStatusChanged();

    std::time_t tt = std::chrono::system_clock::to_time_t(m_ScanStartTimePTP);
    std::tm tm;
#ifdef PLATFORM_WINDOWS
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm)
#endif // PLATFORM_WINDOWS

    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);

    char full[80];
    std::snprintf(full, sizeof(full), "%s.%03lld", buf, static_cast<long long>(ms_fraction.count()));
    LOG_INFO_TAG(m_Name, "Scan Started at {}", full);
}

void mps::Mps::StopScan()
{
    if (!m_Scanning)
        return;

    LOG_INFO_TAG(m_Name, "Scan Complete");

    m_Scanning = false;

    if (!(m_Status == MPS_STATUS_READY))
    {
        m_Status   = MPS_STATUS_READY;

        if (m_OnStatusChanged)
            m_OnStatusChanged();
    }

}

void mps::Mps::ScanThreadFn()
{
    // - pressure/temperature data
    int32_t counts[MPS_MAX_PRESSURE_CH_COUNT];
    int64_t counts_accumulator[MPS_MAX_PRESSURE_CH_COUNT];
    float p[MPS_MAX_PRESSURE_CH_COUNT];
    float t[MPS_MAX_TEMPERATURE_CH_COUNT];

    while (m_Running)
    {
        if (m_Calibrating)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            m_Calibrating = false;
            m_Status      = MPS_STATUS_READY;

            if (m_OnStatusChanged)
                m_OnStatusChanged();
        }

        // - frame start is initially equal to the scan start time
        //   then updated each frame to match the previous frames
        //   end time
        auto frame_start = m_ScanStartTime;
        uint32_t frame = 1;

        while (m_Scanning)
        {

            // - calculate the target end time of this frame
            auto frame_end = frame_start + std::chrono::nanoseconds((int)(1e9 / m_cfg.scan_rate_hz));
            auto dt        = frame_start - m_ScanStartTime;
            auto dt_s      = std::chrono::duration_cast<std::chrono::seconds>(dt);
            auto dt_ns     = std::chrono::duration_cast<std::chrono::nanoseconds>(dt - dt_s);

            std::memset(counts, 0, MPS_MAX_PRESSURE_CH_COUNT * sizeof(int32_t));
            std::memset(counts_accumulator, 0, MPS_MAX_PRESSURE_CH_COUNT * sizeof(int64_t));

            // - clear the pressure array
            std::memset(p, 0, MPS_MAX_PRESSURE_CH_COUNT * sizeof(float));

            // - clear the temperature array
            std::memset(t, 0, MPS_MAX_TEMPERATURE_CH_COUNT * sizeof(float));

            // - timing the subframe, initilze on frame time and 
            auto subframe_start = frame_start;

            for (size_t i = 0; i < (size_t)m_nSubFrameAverages; ++i)
            {
                auto subframe_end = subframe_start + std::chrono::nanoseconds((int)(1e9 / (m_cfg.scan_rate_hz * m_nSubFrameAverages)));

                for (int j = 0; j < (int)m_PhysicalPressureChannelCount; ++j)
                {
                    int32_t sample = RandomSampleCounts();
                    counts_accumulator[j] += sample;
                }

                std::this_thread::sleep_until(subframe_end);
                subframe_start = subframe_end;
            }



            // - resolve the averages and convert to EU
            for (size_t i = 0; i < m_PhysicalPressureChannelCount; ++i)
            {
                 int64_t avg = counts_accumulator[i] / static_cast<int64_t>(m_nSubFrameAverages);
                 avg = std::clamp(avg, static_cast<int64_t>(MPS_MIN_SIGNED_ADC_COUNTS), static_cast<int64_t>(MPS_MAX_SIGNED_ADC_COUNTS));

                 counts[i] = static_cast<int32_t>(avg);
                 p[i] = static_cast<float>(counts[i]) / static_cast<float>(MPS_MAX_SIGNED_ADC_COUNTS) * ::mps_units_conversion_factor(m_cfg.units);

            }

            for (size_t i = 0; i < m_PhysicalTemperatureChannelCount; ++i)
                 t[i] = 32.0f + static_cast<float>(counts[i]) / static_cast<float>(MPS_MAX_SIGNED_ADC_COUNTS);

            if (m_UseLabview)
            {
                LabviewBuf lv{};
                size_t lv_size = 0;
 
                // - first element is the frame (as a float)
                lv.values[0] = static_cast<float>(frame);

                // - calculate average temperatures -> store in secodn element
                for (size_t i = 0; i < m_PhysicalTemperatureChannelCount; ++i)
                    lv.values[1] += t[i];
                lv.values[1] /= m_PhysicalTemperatureChannelCount;

                // - pressures into lv.values + 2
                std::memcpy(lv.values + 2, p, m_PacketPressureChannelCount * sizeof(float));

                // - depending on the DeviceID and SIM setting, we can send the data
                if (m_PacketPressureChannelCount == 64)
                    lv_size = MPS_PKT_64_LABVIEW_SIZE;
                else if(m_PacketPressureChannelCount == 32)
                    lv_size = MPS_PKT_32_LABVIEW_SIZE;
                else if(m_PacketPressureChannelCount == 16)
                    lv_size = MPS_PKT_16_LABVIEW_SIZE;

                if (lv_size > 0 && m_OnLabviewFrame)
                    m_OnLabviewFrame(aero::Buffer::Copy(lv.bytes, lv_size));
            }


            if (m_UseBinary)
            {
                alignas(4) uint8_t packet[MPS_MAX_BINARY_PACKET_SIZE];
                size_t packet_size = 0;
                switch (m_PacketType)
                {
                    case MPS_PKT_LEGACY_TYPE:
                    {
                        auto* data = reinterpret_cast<MpsLegacyPacket*>(packet);
                        std::memset(data, 0, sizeof(MpsLegacyPacket));

                        if (m_UseRaw)
                            std::memcpy(&data->counts, counts, m_PacketPressureChannelCount * sizeof(int32_t));
                        else
                            std::memcpy(&data->pressure, p, m_PacketPressureChannelCount * sizeof(float));

                        std::memcpy(&data->temperature, t, m_PacketTemperatureChannelCount * sizeof(float));

                        data->type                     = m_PacketType;
                        data->size                     = static_cast<int32_t>(sizeof(MpsLegacyPacket));
                        data->frame                    = frame;
                        data->serial_number            = m_cfg.serial_number;
                        data->framerate                = m_cfg.scan_rate_hz;
                        data->valve_status             = GetValveStatus();
                        data->unit_index               = static_cast<uint32_t>(m_cfg.units);
                        data->unit_conversion          = ::mps_units_conversion_factor(m_cfg.units);
                        data->ptp_scan_start_time_s    = m_ScanStartTimePTPSeconds;
                        data->ptp_scan_start_time_ns   = m_ScanStartTimePTPNanoseconds;
                        data->external_trigger_time_us = 0;
                        data->frame_time_s             = static_cast<uint32_t>(dt_s.count());
                        data->frame_time_ns            = static_cast<uint32_t>(dt_ns.count());
                        data->external_trigger_time_s  = 0;
                        data->external_trigger_time_ns = 0;

                        packet_size = m_PacketInfo ? m_PacketInfo->size_bytes : sizeof(MpsLegacyPacket);
                        break;
                    }
                        
                    case MPS_PKT_16_TYPE:
                    {
                        auto* data = reinterpret_cast<Mps16Packet*>(packet);
                        std::memset(data, 0, sizeof(Mps16Packet));
                        std::memcpy(&data->pressure, p, m_PacketPressureChannelCount * sizeof(float));
                        std::memcpy(&data->temperature, t, m_PacketTemperatureChannelCount * sizeof(float));
                        data->type = m_PacketType;
                        data->frame = frame;
                        data->frame_time_s = static_cast<uint32_t>(dt_s.count());
                        data->frame_time_ns = static_cast<uint32_t>(dt_ns.count());

                        packet_size = m_PacketInfo ? m_PacketInfo->size_bytes : sizeof(Mps16Packet);
                        break;
                    }
                    case MPS_PKT_32_TYPE:
                    {
                        auto* data = reinterpret_cast<Mps32Packet*>(packet);
                        std::memset(data, 0, sizeof(Mps32Packet));
                        std::memcpy(&data->pressure, p, m_PacketPressureChannelCount * sizeof(float));
                        std::memcpy(&data->temperature, t, m_PacketTemperatureChannelCount * sizeof(float));
                        data->type = m_PacketType;
                        data->frame = frame;
                        data->frame_time_s = static_cast<uint32_t>(dt_s.count());
                        data->frame_time_ns = static_cast<uint32_t>(dt_ns.count());
                        packet_size = m_PacketInfo ? m_PacketInfo->size_bytes: sizeof(Mps32Packet);
                        break;
                    }
                    case MPS_PKT_64_TYPE:
                    {
                        auto* data = reinterpret_cast<Mps64Packet*>(packet);
                        std::memset(data, 0, sizeof(Mps64Packet));
                        std::memcpy(&data->pressure, p, m_PacketPressureChannelCount * sizeof(float));
                        std::memcpy(&data->temperature, t, m_PacketTemperatureChannelCount * sizeof(float));
                        data->type = m_PacketType;
                        data->frame = frame;
                        data->frame_time_s = static_cast<uint32_t>(dt_s.count());
                        data->frame_time_ns = static_cast<uint32_t>(dt_ns.count());
                        packet_size = m_PacketInfo ? m_PacketInfo->size_bytes : sizeof(Mps64Packet);
                        break;
                    }
                    case MPS_PKT_16_RAW_TYPE:
                    {
                        auto* data = reinterpret_cast<Mps16RawPacket*>(packet);
                        std::memset(data, 0, sizeof(Mps16RawPacket));
                        std::memcpy(&data->counts, counts, m_PacketPressureChannelCount * sizeof(int32_t));
                        std::memcpy(&data->temperature, t, m_PacketTemperatureChannelCount * sizeof(float));
                        data->type = m_PacketType;
                        data->frame = frame;
                        data->frame_time_s = static_cast<uint32_t>(dt_s.count());
                        data->frame_time_ns = static_cast<uint32_t>(dt_ns.count());
                        packet_size = m_PacketInfo ? m_PacketInfo->size_bytes : sizeof(Mps16RawPacket);
                        break;
                    }
                    case MPS_PKT_32_RAW_TYPE:
                    {
                        auto* data = reinterpret_cast<Mps32RawPacket*>(packet);
                        std::memset(data, 0, sizeof(Mps32RawPacket));
                        std::memcpy(&data->counts, counts, m_PacketPressureChannelCount * sizeof(int32_t));
                        std::memcpy(&data->temperature, t, m_PacketTemperatureChannelCount * sizeof(float));
                        data->type = m_PacketType;
                        data->frame = frame;
                        data->frame_time_s = static_cast<uint32_t>(dt_s.count());
                        data->frame_time_ns = static_cast<uint32_t>(dt_ns.count());
                        packet_size = m_PacketInfo ? m_PacketInfo->size_bytes : sizeof(Mps32RawPacket);
                        break;
                    }
                    case MPS_PKT_64_RAW_TYPE:
                    {
                        auto* data = reinterpret_cast<Mps64RawPacket*>(packet);
                        std::memset(data, 0, sizeof(Mps64RawPacket));
                        std::memcpy(&data->counts, counts, m_PacketPressureChannelCount * sizeof(int32_t));
                        std::memcpy(&data->temperature, t, m_PacketTemperatureChannelCount * sizeof(float));
                        data->type = m_PacketType;
                        data->frame = frame;
                        data->frame_time_s = static_cast<uint32_t>(dt_s.count());
                        data->frame_time_ns = static_cast<uint32_t>(dt_ns.count());
                        packet_size = m_PacketInfo ? m_PacketInfo->size_bytes : sizeof(Mps64RawPacket);
                        break;
                    }
                    case MPS_PKT_UNDEFINED:
                        break;

                }

                if (packet_size > 0 && m_OnFrame)
                    m_OnFrame(aero::Buffer::Copy(packet, packet_size));
            }


            std::this_thread::sleep_until(frame_end);
            frame_start = frame_end;

            if (m_cfg.frames_per_scan && (m_cfg.frames_per_scan == frame))
                this->StopScan();

            frame += 1;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    }
}

int32_t mps::Mps::RandomSampleCounts()
{
    double value = m_NormalDistribution(m_RandomGenerator);
    value = std::clamp(value, static_cast<double>(MPS_MIN_SIGNED_ADC_COUNTS), static_cast<double>(MPS_MAX_SIGNED_ADC_COUNTS));

    return static_cast<int32_t>(value);

}

/*
Commands can come from TCP clients, TelNet clients, or a .cfg file.
These should be first split into separate commands (i.e., by \r\n) before
passing to this function.

This function will then further divide each command into tokens and parse them
according the the requested action.

p
*/
std::string mps::Mps::ParseCommands(std::string cmd)
{

    std::vector<std::string> tokens = utils::SplitString(cmd, " \r\n");

    if (tokens[0] == "VER" || tokens[0] == "ver" || tokens[0] == "VERSION" || tokens[0] == "version")
        return std::string("Aiolos (c) MPS Emulator v") + MPS_EMULATOR_VERSION_STRING + ": Emulating Scanivalve Firmware v" + MPS_FIRMWARE_VERSION_STRING + "\r\n>";
    // return "Aiolos (c) MPS Server Emulator v.2024.0\r\n>";

    if (tokens[0] == "STATUS" || tokens[0] == "status")
        return std::string("STATUS: ") + std::string(this->GetStatusStr()) + "\r\n>";

    if (tokens[0] == "CALZ" || tokens[0] == "calz")
    {
        this->CalZ();
        return "";
    }

    if (tokens[0] == "SCAN" || tokens[0] == "scan")
    {
        this->StartScan();
        return "Scan started ..";
    }

    if (tokens[0] == "STOP" || tokens[0] == "stop" || tokens[0][0] == 0x1b) // <ESC>
    {
        this->StopScan();
        return "";
    }

    // ----- S E T ----------------------------------------------------------------------

    if (tokens[0] == "SET" || tokens[0] == "set")
    {
        if (tokens.size() < 2)
            return "Invalid Command: `SET` requires arguments\r\n>";

        if (tokens[1] == "RATE" || tokens[1] == "rate")
        {
            // [0] [1]   [2]   
            // SET RATE <RATE> 
            if (tokens.size() < 3)
                return "Invalid Command: `SET RATE` requires at least 1 arguments: SET RATE <rate>\r\n>";

            if (tokens.size() >= 3)
                std::sscanf(tokens[2].c_str(), "%f", &m_cfg.scan_rate_hz);

            UpdateScanAverages();

            LOG_INFO_TAG(m_Name, "Frame Rate: {}, Internal Averages : {}", m_cfg.scan_rate_hz, m_nSubFrameAverages);

            return cmd + "\r\n>";
        }

        if (tokens[1] == "FPS" || tokens[1] == "fps")
        {

            // [0] [1]   [2]
            // SET FPS <fps>
            if (tokens.size() < 3)
                return "Invalid Command: `SET FPS` requires 1 argument: SET FPS <fps>\r\n>";

            std::sscanf(tokens[2].c_str(), "%d", &m_cfg.frames_per_scan);
            LOG_INFO_TAG(m_Name, "Frames Per Scan: {}", m_cfg.frames_per_scan);
            return cmd + "\r\n>";
        }

        if (tokens[1] == "ENUDP" || tokens[1] == "enudp")
        {

            if (tokens.size() < 3)
                return "Invalid Command: `SET ENUDP` requires 1 argument: SET ENUDP <0 or 1>\r\n>";

            int enudp;
            std::sscanf(tokens[2].c_str(), "%d", &enudp);
            m_cfg.udp_enabled = (bool)enudp;

            LOG_INFO_TAG(m_Name, "UDP enabled = {}", m_cfg.udp_enabled);
            return cmd + "\r\n>";
        }
        if (tokens[1] == "ENFTP" || tokens[1] == "ENFTP")
        {

            if (tokens.size() < 3)
                return "Invalid Command: `SET FTP` requires 1 argument: SET ENUDP <0 or 1>\r\n>";

            int enftp;
            std::sscanf(tokens[2].c_str(), "%d", &enftp);
            m_cfg.ftp_enabled = (bool)enftp;

            LOG_INFO_TAG(m_Name, "FTP enabled = {}", m_cfg.ftp_enabled);
            return cmd + "\r\n>";
        }

        if (tokens[1] == "IPUDP" || tokens[1] == "ipudp")
        {
            if (tokens.size() < 4)
                return "Invalid Command: `SET IPUDP` requires 2 argument: SET IPUDP <ip> <port>\r\n>";

            m_cfg.udp_target_ip = tokens[2];
            std::sscanf(tokens[3].c_str(), "%hd", &m_cfg.udp_target_port);
            return cmd + "\r\n>";
        }

        if (tokens[1] == "FORMAT" || tokens[1] == "format")
        {
            for (int i = 2; i < tokens.size(); i += 2)
            {
                
                if (i + 1 >= tokens.size())
                    return "Invalid Command: `SET FORMAT` requires argument pairs: e.g. SET FORMAT F <format>\r\n>";

                if (!SetDataFormat(tokens[i][0], tokens[i + 1][0]))
                    return "Invalid Command: SET FORMAT " + tokens[i] + " " + tokens[i + 1] + "\r\n>";
            }

            return "SET FORMAT" + BuildDataFormatResponse() + "\r\n>";
        }

        if (tokens[1] == "UNITS" || tokens[1] == "units")
        {
            if (tokens.size() < 3)
                return "Invalid Command: `SET UNITS` requires 1 argument: SET UNITS <UNIT>\r\n>";

            if (!::mps_units_from_string(tokens[2].c_str(), &m_cfg.units))
                return "Invalid Command: SET UNITS " + tokens[2] + " (case sensitive)\r\n>";

            LOG_INFO_TAG(
                m_Name, "Units {} set with conversion {}", ::mps_units_to_string(m_cfg.units),
                ::mps_units_conversion_factor(m_cfg.units)
            );
            return cmd + "\r\n>";
        }

        if (tokens[1] == "SIM" || tokens[1] == "sim")
        {
            if (tokens.size() < 3)
                return "Invalid Command: `SET SIM` requires 1 argument: SET SIM <SIM>\r\n>";


            const char* s = tokens[2].c_str();
            char* end = nullptr;
            
            unsigned long value = std::strtoul(s,&end, 0);

            if (end == s)
                return "Invalid Command: `SET SIM` requires 1 argument: SET SIM <SIM>\r\n>";

            if (*end != '\0')
                return std::string("Invalid Command: `SET SIM`") + cmd + "\r\n>";

            if (errno == ERANGE || value > 65535ul)
                return std::string("Invalid Command: ") + cmd + " Value out of range [0-65535]\r\n>";

            m_cfg.sim = static_cast<MpsSimFlags>(static_cast<uint16_t>(value));
            return cmd + "\r\n>";

        }
    }

    // ----- G E T ----------------------------------------------------------------------
    if (tokens[0] == "GET" || tokens[0] == "get")
    {
        if (tokens.size() < 2)
            return "Invalid Command: `GET` requires arguments\r\n>";

        if (tokens[1] == "SN" || tokens[1] == "sn")
        {
            std::stringstream out;
            out << "SET SN "<< m_cfg.serial_number << "\r\n>";
            return out.str();
        }

        if (tokens[1] == "RATE" || tokens[1] == "rate")
        {
            std::stringstream out;
            out << "SET RATE "<< m_cfg.scan_rate_hz << "\r\n>"; 
            return out.str();
        }

        if (tokens[1] == "UNITS" || tokens[1] == "units")
        {
            std::stringstream out;
            out << "SET UNITS "<< ::mps_units_to_string(m_cfg.units) << " " << ::mps_units_conversion_factor(m_cfg.units) << "\r\n>";
            return out.str();
        }
        if (tokens[1] == "FPS" || tokens[1] == "fps")
        {
            std::stringstream out;
            out << "SET FPS "<< m_cfg.frames_per_scan << "\r\n>";
            return out.str();
        }
    }

    // ----- L I S T --------------------------------------------------------------------

    if (tokens[0] == "LIST" || tokens[0] == "list")
    {
        if (tokens.size() < 2)
            return "Invalid Command: `LIST` requires arguments\r\n>";

        if (tokens[1] == "S" || tokens[1] == "s")
        {
            //- TODO(Chris): these should just read the cfg files and return the commands stored there.
            // build a string stream response.
            std::stringstream out;
            out << "SET RATE " << m_cfg.scan_rate_hz << "\r\n";
            out << "SET FPS " << m_cfg.frames_per_scan << "\r\n";
            out << "SET UNITS " << ::mps_units_to_string(m_cfg.units) << " " << ::mps_units_conversion_factor(m_cfg.units) << "\r\n";
            out << "SET FORMAT";
            out << BuildDataFormatResponse() << "\r\n";
            out << "SET TRIG 0 \r\n";                      //- TODO(Chris): implement
            out << "SET ENFTP " << m_cfg.ftp_enabled << " \r\n"; //
            // out << "SET OPTIONS 0 0 16 \r\n";              //- TODO(Chris): removed in v4
            out << ">";

            return out.str();
        }
        if (tokens[1] == "UDP" || tokens[1] == "udp")
        {
            // build a string stream response. TBD
            std::stringstream out;
            out << "SET ENUDP " << (int)m_cfg.udp_enabled << "\r\n";
            out << "SET IPUDP " << m_cfg.udp_target_ip << " " << m_cfg.udp_target_port << "\r\n";
            out << ">";

            return out.str();
        }
        if (tokens[1] == "M" || tokens[1] == "m")
        {
            std::stringstream out;
            out << "SET SIM " << (uint16_t)m_cfg.sim << "\r\n";
            out << "SET ECHO 1\r\n";
            out << "SET XITE 2\r\n";
            out << "SET ETOL 0\r\n";
            out << ">";

            return out.str();

        }
    }

    LOG_ERROR_TAG(m_Name, "Invalid Command: `{}`", cmd);
    return "Invalid Command: `" + cmd + "`\r\n>";
}

