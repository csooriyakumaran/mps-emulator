#ifndef _MPS_H_
#define _MPS_H_

#include <stdint.h>


namespace mps
{

struct BinaryPacket
{
        int32_t type;
        int32_t size; // 348 for BinaryPacket
        int32_t frame;
        int32_t serial_number;
        float frame_rate;     // Hz ... #TODO how do i ensure that this is only 4 bytes?
        int32_t valve_status; // (0: Px, 1: Cal)
        int32_t unit_index;   // see appendix E of the MPS manual (0: psi, 23: Pa)
        float unit_conversion;
        uint32_t ptp_scan_start_time_s;    // sec
        uint32_t ptp_scan_start_time_ns;   // nanosections
        uint32_t external_trigger_time_us; // microseconds?
        float temperature[8];
        float pressure[64];
        uint32_t frame_time_s;
        uint32_t frame_time_ns;
        uint32_t external_trigger_time_s;
        uint32_t external_trigger_time_ns;
};

struct ScannerConfig
{
    uint32_t serial_number;
    float frame_rate:

};

} // namespace mps

#endif // _MPS_H_
