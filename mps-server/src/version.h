#ifndef MPS_SERVER_VERSION_H_
#define MPS_SERVER_VERSION_H_

#define VERSION_MAJOR 2024
#define VERSION_MINOR 0

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

#define VERSION_STRING \
    STRINGIFY(VERSION_MAJOR) "." STRINGIFY(VERSION_MINOR)

inline constexpr int VersionMajor = VERSION_MAJOR;
inline constexpr int VersionMinor = VERSION_MINOR;
inline constexpr const char* VersionString = VERSION_STRING;


#endif // MPS_SERVER_VERSION_H_

