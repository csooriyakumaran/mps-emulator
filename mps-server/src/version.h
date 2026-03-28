#ifndef MPS_SERVER_VERSION_H_
#define MPS_SERVER_VERSION_H_

#define VERSION_MAJOR 0
#define VERSION_MINOR 2
#define VERSION_PATCH 0
#define VERSION_SUFFIX "-dev"

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

#define VERSION_STRING \
    STRINGIFY(VERSION_MAJOR) "." \
    STRINGIFY(VERSION_MINOR) "." \
    STRINGIFY(VERSION_PATCH) VERSION_SUFFIX

inline constexpr int VersionMajor = VERSION_MAJOR;
inline constexpr int VersionMinor = VERSION_MINOR;
inline constexpr int VersionPatch = VERSION_PATCH;
inline constexpr const char* VersionSuffic = VERSION_SUFFIX;
inline constexpr const char* VersionString = VERSION_STRING;


#endif // MPS_SERVER_VERSION_H_

