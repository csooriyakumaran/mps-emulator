#ifndef _AERO_CORE_LOG_H_
#define _AERO_CORE_LOG_H_

#include <chrono>
#include <format>
#include <iostream>
#include <stdint.h>
#include <string>
#include <string_view>
#include <syncstream>
#include <vector>

namespace aero
{
namespace Log
{
enum class Level : uint8_t
{
    Debug = 0,
    Info,
    Warn,
    Error,
    _COUNT
};

static std::string LevelNames[(size_t)Level::_COUNT] = {"DEGUB", "INFO", "WARN", "ERROR"};
static std::string LevelColor[(size_t)Level::_COUNT] = {"92m", "94m", "93m", "91m"};

template<typename... Args>
static void PrintMsg(Log::Level level, std::string_view fmt, std::string_view tag, Args&&... args);
template<typename... Args>
static void PrintAssertMsg(std::string_view tag, std::string_view fmt="", Args&&... args);
}; // namespace Log

} // namespace aero

#if DEBUG
    #define LOG_DEBUG(fmt, ...) ::aero::Log::PrintMsg(::aero::Log::Level::Debug, fmt,  "", __VA_ARGS__)
    #define LOG_DEBUG_TAG(tag, fmt, ...) ::aero::Log::PrintMsg(::aero::Log::Level::Debug, fmt, tag, __VA_ARGS__)
#else
    #define LOG_DEBUG
    #define LOG_DEBUG_TAG
#endif // DEBUG

#define LOG_INFO(fmt, ...) ::aero::Log::PrintMsg(::aero::Log::Level::Info, fmt, "", __VA_ARGS__)
#define LOG_WARN(fmt, ...) ::aero::Log::PrintMsg(::aero::Log::Level::Warn, fmt, "", __VA_ARGS__)
#define LOG_ERROR(fmt, ...) ::aero::Log::PrintMsg(::aero::Log::Level::Error, fmt, "", __VA_ARGS__)

#define LOG_INFO_TAG(tag,fmt,  ...) ::aero::Log::PrintMsg(::aero::Log::Level::Info, fmt, tag, __VA_ARGS__)
#define LOG_WARN_TAG(tag,fmt,  ...) ::aero::Log::PrintMsg(::aero::Log::Level::Warn, fmt, tag, __VA_ARGS__)
#define LOG_ERROR_TAG(tag,fmt,  ...) ::aero::Log::PrintMsg(::aero::Log::Level::Error, fmt, tag, __VA_ARGS__)

template<typename... Args>
void aero::Log::PrintMsg(aero::Log::Level level, std::string_view fmt, std::string_view tag, Args&&... args)
{

    std::chrono::time_point t = std::chrono::system_clock::now();
    std::string time          = std::format("{:%Y-%m-%d %H:%M:%OS}", t);
    std::string loglevel      = Log::LevelNames[(size_t)level];
    std::string logcolor      = Log::LevelColor[(size_t)level];

    std::string tagString;
    if (tag.empty())
        tagString = std::format(" [ \033[{}{:5}\033[0m ] ", logcolor, loglevel);
    else
        tagString = std::format(" [ \033[{}{:5}\033[0m ] :: \033[{}{}\033[0m :: ", logcolor, loglevel, logcolor, tag);

    std::string msg = std::vformat(fmt, std::make_format_args(args...));

    std::osyncstream out{std::cout};
    out << time << tagString << msg << std::endl;
}
template<typename... Args>
void aero::Log::PrintAssertMsg(std::string_view tag, std::string_view fmt, Args&&... args)
{
    return aero::Log::PrintMsg(aero::Log::Level::Error, fmt, tag, std::forward<Args>(args)...);
    /*std::chrono::time_point t = std::chrono::system_clock::now();*/
    /*std::string time          = std::format("{:%Y-%m-%d %H:%M:%OS}", t);*/
    /**/
    /*aero::Log::Level level = aero::Log::Level::Error;*/
    /*std::string loglevel      = Log::LevelNames[(size_t)level];*/
    /*std::string logcolor      = Log::LevelColor[(size_t)level];*/
    /**/
    /*std::string tagString;*/
    /*if (tag.empty())*/
    /*    tagString = std::format(" [ \033[{}{:5}\033[0m ] ", logcolor, loglevel);*/
    /*else*/
    /*    tagString = std::format(" [ \033[{}{:5}\033[0m ] :: \033[{}{}\033[0m :: ", logcolor, loglevel, logcolor, tag);*/
    /**/
    /*std::string msg = std::vformat(fmt, std::make_format_args(args...));*/
    /**/
    /*std::osyncstream out{std::cout};*/
    /*out << time << tagString << msg << std::endl;*/
}

template<>
inline void aero::Log::PrintAssertMsg(std::string_view tag, std::string_view fmt)
{
    return aero::Log::PrintMsg(aero::Log::Level::Error, fmt, tag);
}
#endif // _AERO_CORE_LOG_H_
