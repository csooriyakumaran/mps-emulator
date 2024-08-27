#ifndef _SCANIVALVE_MPS_CONFIG_H_
#define _SCANIVALVE_MPS_CONFIG_H_

#include <stdint.h>
#include <string>

namespace mps
{

enum class ScanUnits : int32_t
{
    PSI = 0,
    ATM,
    BAR,
    CMHG,
    CMH2O,
    DECIBAR,
    FTH2O,
    GCM2,
    INHG,
    INH2O,
    KGCM2,
    KGM2,
    KIPIN2,
    KNM2,
    KPA,
    MBAR,
    MH2O,
    MMHG,
    MPA,
    NCM2,
    NM2,
    OZFT2,
    OZIN2,
    PA,
    PSF,
    TORR,
    USER,
    RAW,
    _COUNT
};

static std::string ScanUnitsNames[(size_t)ScanUnits::_COUNT] = {
    "PSI",   "ATM",   "BAR",    "CMHG", "CMH2O", "DECIBAR", "FTH2O", "GCM2", "INHG", "INH2O",
    "KGCM2", "KGM2",  "KIPIN2", "KNM2", "KPA",   "MBAR",    "MH2O",  "MMHG", "MPA",  "NCM2",
    "NM2",   "OZFT2", "OZIN2",  "PA",   "PSF",   "TORR",    "USER",  "RAW",
};

static float ScanUnitConversion[(size_t)ScanUnits::_COUNT] = {
    1.0f,         // PSI = 0,
    0.068046f,    // ATM,
    0.068947f,    // BAR,
    5.17149f,     // CMHG,
    70.308f,      // CMH2O,
    0.68947f,     // DECIBAR,
    2.3067f,      // FTH2O,
    70.306f,      // GCM2,
    2.0360f,      // INHG,
    27.680f,      // INH2O,
    0.070307f,    // KGCM2,
    703.069f,     // KGM2,
    0.001f,       // KIPIN2,
    6.89476f,     // KNM2,
    6.89476f,     // KPA,
    68.947f,      // MBAR,
    0.70309f,     // MH2O,
    51.7149f,     // MMHG,
    0.00689476f,  // MPA,
    0.689476f,    // NCM2,
    6894.759766f, // NM2,
    2304.00f,     // OZFT2,
    16.0f,        // OZIN2,
    6894.759766f, // PA,
    144.00f,      // PSF,
    51.71490f,    // TORR,
    1.0f,         // USER,
    1.0f,         // RAW,
};

/*template<typename T>*/
/*T Load(std::filesystem::path fname)*/
/*{*/
/*    if (!std::filesystem::exists(fname))*/
/*    {*/
/*        LOG_ERROR_TAG("MPS-CFG", "Config file {} does not exist, create with SAVE cmd",
 * fname.filename());*/
/*        return T(); // default*/
/*    }*/
/**/
/*    std::ifstream f(fname.string().c_str(), std::ios::in | std::ios::binary);*/
/**/
/*    if (!f)*/
/*    {*/
/*        LOG_ERROR_TAG("MPS-CFG", "Could not open config file {}", fname.filename());*/
/*        return T(); // return default*/
/*    }*/
/**/
/*    T out;*/
/**/
/*    auto fsize = std::filesystem::file_size(fname);*/
/*    ASSERT(fsize == sizeof(T), "Config file size {} is not the right size of {} bytes", fsize,
 * sizeof(T));*/
/*    f.read((char*)&out, sizeof(T));*/
/**/
/*    f.close();*/
/**/
/*    return out;*/
/**/
/*}*/

} // namespace mps

#endif // _SCANIVALVE_MPS_CONFIG_H_
