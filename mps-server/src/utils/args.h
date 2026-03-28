#ifndef _MPS_ARGS_H_
#define _MPS_ARGS_H_

#include "aero/core/log.h"
#include <iostream>
#include <stdlib.h>
#include <string_view>
#include <vector>

#include "version.h"

namespace args
{

struct Options
{
    uint32_t device_type = 4264;
    uint16_t port        = 23;
    std::string bind_ip  = "127.0.0.1";
    bool should_close    = false;
    bool enable_console  = true;
};

static bool has_option_flag(const std::vector<std::string_view>& args, const std::string_view name)
{
    for (auto it = args.begin(), end = args.end(); it != end; ++it)
        if (*it == name)
            return true;

    return false;
}

static std::string_view get_option_value(const std::vector<std::string_view>& args, const std::string_view name)
{
    for (auto it = args.begin(), end = args.end(); it != end; ++it)
        if (*it == name)
            if (it + 1 != end)
                return *(it + 1);

    return "";
}

template<typename T>
bool arg_to_num(const std::string_view& opt, T& out)
{
    if (opt.empty())
        return false;

    const char* first = opt.data();
    const char* last  = opt.data() + opt.length();

    std::from_chars_result res = std::from_chars(first, last, out);

    if (res.ec != std::errc())
        return false;
    if (res.ptr != last)
        return false;

    return true;
}

static void PrintHelp(char* name)
{

    std::cout << "\nusage:\n";
    std::cout << "  mps-server.exe ";
    std::cout << "[--help] ";
    std::cout << "[--version] ";
    std::cout << "[--enable-console] ";
    std::cout << "[--type <device-type>] ";
    std::cout << "[--port <port>] ";
    std::cout << "[--bind-ip <ip>] ";
    std::cout << "\n\n";

    std::cout << "\nAn emulated MPS4200-series scanner (based on firmware version 4.01)\n";

    std::cout << "\nFlags:\n";
    std::cout << "  -h, --help        \tPrint help message and exit\n";
    std::cout << "  -v, --version     \tPrint version number and exit\n";
    std::cout << "  --enable-console  \tEnable embedded console\n";

    std::cout << "\nOptions:\n";
    std::cout << "  -t, --type <TYPE> \tMPS Type (default: 4264) [ ALLOWED: 4264 | 4232 | 4216 ]\n";
    std::cout << "  -p, --port <PORT> \tTCP port (default: 23)\n";
    std::cout << "  --bind-ip  <IP>   \tIP address to bind (default: 127.0.0.1)\n";
}

static Options ParseAguments(int argc, char** argv)
{
    Options opts;

    const std::vector<std::string_view> args(argv, argv + argc);
    std::string_view option_val;

    if (has_option_flag(args, "-v") || has_option_flag(args, "--version"))
    {
        std::cout << "Aiolos (c) MPS Server v" << VersionString << '\n';
        opts.should_close = true;
        return opts;
    }

    if (has_option_flag(args, "-h") || has_option_flag(args, "--help"))
    {
        PrintHelp(argv[0]);
        opts.should_close = true;
        return opts;
    }

    if (has_option_flag(args, "-t"))
    {
        uint32_t device_type;
        option_val = get_option_value(args, "-t");
        
        if(arg_to_num(option_val, device_type))
            opts.device_type = device_type;
        else
            LOG_ERROR("Option flag -t must be followed by a numeric argument");
    }

    if (has_option_flag(args, "--type"))
    {
        uint32_t device_type;
        option_val = get_option_value(args, "--type");
        
        if(arg_to_num(option_val, device_type))
            opts.device_type = device_type;
        else
            LOG_ERROR("Option flag --type must be followed by a numeric argument");
    }

    if (has_option_flag(args, "-p"))
    {
        uint16_t port;
        option_val = get_option_value(args, "-p");

        if (arg_to_num(option_val, port))
            opts.port = port;
        else
            LOG_ERROR("Option flag -p must be followed by a numeric argument");
    }

    if (has_option_flag(args, "--port"))
    {
        uint16_t port;
        option_val = get_option_value(args, "--port");

        if (arg_to_num(option_val, port))
            opts.port = port;
        else
            LOG_ERROR("Option flag --port must be followed by a numeric argument");
    }

    if (has_option_flag(args, "--bind-ip"))
    {
        opts.bind_ip = get_option_value(args, "--bind-ip");
    }

    opts.enable_console = has_option_flag(args, "--enable-console");
    return opts;
}

} // namespace args

#endif // _MPS_ARGS_H_
