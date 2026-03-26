#ifndef AERO_NETWORKING_UTILS_H_
#define AERO_NETWORKING_UTILS_H_

#include <stdint.h>
#include <string>
#include <cstring>
#include <winsock2.h>
#include <WS2tcpip.h>


namespace aero
{

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

