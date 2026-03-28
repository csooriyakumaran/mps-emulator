#ifndef __SCANIVALVE_MPS_H_
#define __SCANIVALVE_MPS_H_

#include <chrono>
#include <random>
#include <stdint.h>
#include <string>
#include <string_view>

#include "aero/core/buffer.h"
#include "aero/core/threads.h"


#include "scanivalve/mps-protocol.h"

namespace mps
{

// todo(chris): Should this be in the protocol header? do client apps need it?
struct MpsConfig
{
    MpsDeviceID device_id;               /* device ID */
    uint32_t serial_number;       /* SN */

    MpsDataFormat telnet_format;  /* FORMAT T */
    MpsDataFormat ftp_udp_format; /* FORMAT F */
    MpsDataFormat binary_format;  /* FORMAT B */

    float scan_rate_hz;           /* ScanRate */
    uint32_t frames_per_scan;     /* FPS, 0 = continuous */

    MpsUnits units;               /* pressure units */

    bool udp_enabled;              /* ENUDP (0/1) */
    std::string udp_target_ip;       /* dotted-quad string */
    uint16_t udp_target_port;     /* UDP port */

    bool ftp_enabled;              /* ENFTP (0/1) */

    MpsSimFlags sim;
};

union LabviewBuf
{
    float values[MPS_MAX_LABVIEW_ELEMENTS];
    uint8_t bytes[sizeof(values)];
};

using FrameCallback  = std::function<void(aero::Buffer)>;
using StatusCallback = std::function<void(void)>;

class Mps
{
public:
    Mps(const MpsConfig& cfg, FrameCallback on_frame = nullptr, FrameCallback on_labview = nullptr, StatusCallback on_scan_state_changed = nullptr);
    ~Mps();

    void Start();
    void Shutdown();
    void StartScan();
    void StopScan();

    void CalZ();
    std::string ParseCommands(std::string cmds);

    // const std::string& GetStatusStr() const { return StatusStr[(size_t)m_Status]; }
    std::string_view GetStatusStr() const;
    const MpsStatus& GetStatus() const { return m_Status; }
    int32_t GetValveStatus() const { return 0; }

    MpsConfig& GetConfig() { return m_cfg; }
    const MpsConfig& GetConfig() const { return m_cfg; }
    void SetConfig(MpsConfig cfg) { m_cfg = cfg; }

private:
    void ScanThreadFn();
    int32_t RandomSampleCounts();
    float CountsToFloat(int32_t counts) { return static_cast<float>(counts) / static_cast<float>(MPS_MAX_SIGNED_ADC_COUNTS); }
    uint32_t UnitIndex() { return (uint32_t)m_cfg.units; }
    void UpdateScanAverages();
    void ConfigureScanLayout();

    bool SetDataFormat(char type, char fmt);
    std::string BuildDataFormatResponse();

    FrameCallback m_OnFrame = nullptr;
    FrameCallback m_OnLabviewFrame = nullptr;
    StatusCallback m_OnStatusChanged = nullptr;
private:
    std::string      m_Name;
    aero::ThreadPool m_ThreadPool;

    MpsConfig m_cfg;
    MpsStatus m_Status = MPS_STATUS_READY;

    MpsBinaryPacketType m_PacketType   = MPS_PKT_LEGACY_TYPE;
    const MpsBinaryPacketInfo* m_PacketInfo  = nullptr;

    uint32_t m_PhysicalPressureChannelCount    = 64;
    uint32_t m_PhysicalTemperatureChannelCount = 8;
    uint32_t m_PacketPressureChannelCount      = 64;
    uint32_t m_PacketTemperatureChannelCount   = 8;
;
    std::atomic<bool> m_Running     = false;
    std::atomic<bool> m_Scanning    = false;
    std::atomic<bool> m_Calibrating = false;
    std::atomic<bool> m_UseLabview  = false;
    std::atomic<bool> m_UseBinary   = false;
    std::atomic<bool> m_UseRaw      = false;
    std::atomic<bool> m_ZeroPad     = true;

    uint32_t m_nSubFrameAverages = 1;

    // - Random number generator
    std::mt19937 m_RandomGenerator;
    std::normal_distribution<double> m_NormalDistribution;

    // - timing
    std::chrono::time_point<std::chrono::steady_clock> m_ScanStartTime;
    std::chrono::time_point<std::chrono::system_clock> m_ScanStartTimePTP;
    uint32_t m_ScanStartTimePTPSeconds;
    uint32_t m_ScanStartTimePTPNanoseconds;


};

} // namespace mps
// namespace mps

#endif // __SCANIVALVE_MPS_H_
