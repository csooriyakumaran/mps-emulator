#include "console.h"

#include "aero/core/log.h"

Console::Console()
{
    m_InputThread = std::jthread([this]() { this->InputThreadFn(); });
}

Console::~Console()
{
    m_InputThreadRunning = false;

    LOG_WARN_TAG("CONSOLE", "Input thread of the console will hang until it receives user input");
    std::cout << "Press <Enter> to stop or restart the console: ";
    m_MessageCallback = nullptr;
}

void Console::InputThreadFn()
{

    m_InputThreadRunning = true;

    while (m_InputThreadRunning)
    {
        std::string line;
        std::getline(std::cin, line);
        if (m_MessageCallback)
            m_MessageCallback(line);
    }
}
