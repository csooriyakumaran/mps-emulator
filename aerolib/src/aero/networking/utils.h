#ifndef AERO_NETWORKING_UTILS_H_
#define AERO_NETWORKING_UTILS_H_

#include <stdint.h>
#include <string>
#include <cstring>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <span>

#include "aero/core/buffer.h"

namespace aero
{

static inline void HexDump(const aero::Buffer& buf)
{
        auto data = std::span<const uint8_t>(buf.As<uint8_t>(), buf.size);

        size_t width = 16;

        for (size_t i = 0; i < data.size(); i+=width)
        {
            // hex
            for (size_t j = 0; j < width; ++j)
            {
                if (i + j < data.size())
                {
                    auto v = data[i+j];
                printf("%02X ", v);
                if ((i+1) % 16 == 0)
                    printf("\n");
                }
                else
                {
                    printf("    ");
                }
            }

            printf(" | ");

            // ascii
            for (size_t j = 0; j < width && i + j < data.size(); ++j)
            {
                auto v = data[i+j];
                printf("%c", std::isprint(v) ? v : '.');
            }
            printf("\n");
        }
}

static void SwapToNetworkOrderInPlace(aero::Buffer& buf)
{
    uint8_t* data = static_cast<uint8_t*>(buf.data);
    size_t size = buf.size;

    if (size % 4 != 0)
        return;

    uint32_t* words = reinterpret_cast<uint32_t*>(data);
    size_t count = size / 4;

    for (size_t i = 0; i < count; ++i)
        words[i] = htonl(words[i]);

    return;
}

static inline uint32_t HostFloatToBE(float value)
{
    static_assert(sizeof(float) == sizeof(uint32_t), "float must be 32-bit");

    uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    bits = htonl(bits);
    return bits;
}

static inline uint32_t ipv4_string_to_network_u32(const std::string& ip)
{
    in_addr addr{};
    int rc = ::InetPtonA(AF_INET, ip.c_str(), &addr);

    if (! rc) return 0;

    return static_cast<uint32_t>(addr.S_un.S_addr);

}

static inline uint32_t extract_last_octet(uint32_t ip_net)
{
    return (ip_net >> 24) & 0xFFu;
}

static inline uint32_t extract_last_octet(const std::string& ip)
{
    return extract_last_octet(ipv4_string_to_network_u32(ip));
}


} // nampespace aero

#endif // AERO_NETWORKING_UTILS_H_

