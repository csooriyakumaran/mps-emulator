#include "aero/core/application.h"

#include <chrono>
#include <iostream>
#include <thread>

#include <Windows.h>

extern bool g_ApplicationRunning;

static aero::Application* s_Instance = nullptr;

aero::Application::Application(const aero::AppSpec& spec)
    : m_Specification(spec)
{
    s_Instance = this;
}

aero::Application::~Application() { s_Instance = nullptr; }

aero::Application& aero::Application::Get() { return *s_Instance; }

void aero::Application::Init()
{
    // Log::Init();
}

void aero::Application::Shutdown()
{
    Restart();
    g_ApplicationRunning = false;
}

void aero::Application::Restart()
{
    for (auto& layer : m_LayerStack)
        layer->OnDetach();

    m_LayerStack.clear();
    m_Running = false;
}

void aero::Application::Run()
{
    m_Running = true;

    std::cout << "[ APP  ] Starting the main loop ....\n";

    //- main loop
    while (m_Running)
    {

        for (auto& layer : m_LayerStack)
            layer->OnUpdate();

        if (m_Specification.SleepMilliseconds > 0.0f)
            std::this_thread::sleep_for(std::chrono::milliseconds(m_Specification.SleepMilliseconds)
            );

        float time      = m_AppTimer.Elapsed();
        m_FrameTime     = time - m_LastFrameTime;
        m_LastFrameTime = time;

        int cols = 100;
        /*CONSOLE_SCREEN_BUFFER_INFO csbi;*/
        /*int cols, rows;*/
        /*GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);*/
        /*cols             = csbi.srWindow.Right - csbi.srWindow.Left + 1;*/
        /*rows             = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;*/

        float frame_rate = 1.0f / m_FrameTime;
        /*std::cout << std::setw(cols) << std::right << frame_rate << "\r";*/
    }
}
