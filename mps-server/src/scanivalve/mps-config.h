#ifndef _SCANIVALVE_MPS_CONFIG_H_
#define _SCANIVALVE_MPS_CONFIG_H_

#include <filesystem>
#include <stdint.h>
#include <string>
#include <format>
#include <iostream>

#include "aero/core/buffer.h"
#include "aero/core/log.h"
#include "aero/core/assert.h"

namespace mps
{


template<typename T>
T Load(std::filesystem::path fname)
{
    if (!std::filesystem::exists(fname))
    {
        LOG_ERROR_TAG("MPS-CFG", "Config file {} does not exist, create with SAVE cmd", fname.filename());
        return T(); // default
    }
    
    std::ifstream f(fname.string().c_str(), std::ios::in | std::ios::binary);
    
    if (!f)
    {
        LOG_ERROR_TAG("MPS-CFG", "Could not open config file {}", fname.filename());
        return T(); // return default
    }
    
    T out;

    auto fsize = std::filesystem::file_size(fname);
    ASSERT(fsize == sizeof(T), "Config file size {} is not the right size of {} bytes", fsize, sizeof(T));
    f.read((char*)&out, sizeof(T));

    f.close();

    return out;



}


struct IPConfig : public Config
{
    void Load() override;
    void Save() override;
};

struct IDConfig : public Config
{
    void Load() override;
    void Save() override;
};

struct ScanConfig : public Config
{
    float rate;
    int fps;
    std::string units;
    std::
    void Load() override;
    void Save() override;
};

struct PTPConfig : public Config
{
    void Load() override;
    void Save() override;
};

struct FCConfig : public Config
{
    void Load() override;
    void Save() override;
};

struct CALConfig : public Config
{
    CALConfig(int32_t sn)
        : fname(std::format("Cal_{:03d}.cgf", sn))
    {
        Load();
    }
    void Load() override;
    void Save() override;
};

struct MConfig : public Config
{
    std::filesystem::path fname = "hw.cfg";
};

struct FTPConfig : public Config
{
    std::filesystem::path fname = "ftp.cfg";
};

struct OConfig : public Config
{
    std::filesystem::path fname = "oven.cfg";
};


} // namespace mps

#endif // _SCANIVALVE_MPS_CONFIG_H_
