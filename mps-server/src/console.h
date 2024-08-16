#ifndef _MPS_EMULATOR_CONSOLE_H
#define _MPS_EMULATOR_CONSOLE_H

#include <functional>
#include <string_view>
#include <thread>


class Console
{
public:
    using MessageCallback = std::function<void(std::string_view)>;

public:
    Console();
    ~Console();


    void SetMessageCallback(const MessageCallback& fn) { m_MessageCallback = fn; }

private:
    void InputThreadFn();

    std::jthread m_InputThread;

    bool m_InputThreadRunning  = false;

    MessageCallback m_MessageCallback = nullptr;
};
#endif // _MPS_EMULATOR_CONSOLE_H
