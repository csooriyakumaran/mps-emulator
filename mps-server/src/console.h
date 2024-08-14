#ifndef _MPS_EMULATOR_CONSOLE_H
#define _MPS_EMULATOR_CONSOLE_H

#include <format>
#include <iostream>
#include <string>
#include <string_view>

class Console
{
public:
    Console() {}
    ~Console() {}

    template<typename... Args>
    void AddTaggedMsg(std::string_view tag, std::string_view format, Args&&... args)
    {
        std::string msg = std::vformat(format, std::make_format_args(args...));
        std::cout << "[ " << tag << " ] " << msg << std::endl;
    }
    void InputThread();
};
#endif // _MPS_EMULATOR_CONSOLE_H
