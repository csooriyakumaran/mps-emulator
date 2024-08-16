#ifndef _MPS_ARGS_H_
#define _MPS_ARGS_H_

#include <stdlib.h>
#include <iostream>
#include <vector>
#include <string_view>
#include "aero/core/log.h"

namespace args
{

struct Options
{
    uint16_t port = 65432;
    bool should_close = false;
};

static bool has_option_flag(const std::vector<std::string_view>& args, const std::string_view name)
{
    for (auto it = args.begin(), end = args.end(); it != end; ++it)
        if (*it == name)
            return true;

    return false;
}

static std::string_view
get_option_value(const std::vector<std::string_view>& args, const std::string_view name)
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

    const char* first          = opt.data();
    const char* last           = opt.data() + opt.length();

    std::from_chars_result res = std::from_chars(first, last, out);

    if (res.ec != std::errc())
        return false;
    if (res.ptr != last)
        return false;

    return true;
}

static void PrintHelp(char* name)
{

    std::cout << "\nusage:\tmps-server.exe ";

    std::cout << "[-p <port> | --port <port>]\n\n";
    
    std::cout << "\tThis program emulates a Scanivalve MPS4200 series pressure scanner \n";
    std::cout << "\tFor use in developing a client application for communicating with \n";
    std::cout << "\tand receiving data from a pressure scanner. A TCP server is started \n"; 
    std::cout << "\ton the speficied port (or default of 65432). Simulated scan data is \n";
    std::cout << "\tis sent over a UDP socket by default\n";
}

static Options ParseAguments(int argc, char** argv)
{
    Options opts;

    const std::vector<std::string_view> args(argv, argv + argc);
    std::string_view option_val;

    if (has_option_flag(args, "-h") || has_option_flag(args, "--help"))
    {
        PrintHelp(argv[0]);
        opts.should_close = true;
        return opts;
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
    return opts;
}

} // namespace args

#endif // _MPS_ARGS_H_
