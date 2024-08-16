#include "aero/core/application.h"

#include <chrono>
#include <iostream>
#include <thread>

/*#include <Windows.h>*/

#include "aero/core/log.h"

extern bool g_ApplicationRunning;

static aero::Application* s_Instance = nullptr;

aero::Application::Application(const aero::AppSpec& spec)
    : m_Specification(spec)
{
    s_Instance = this;
}

aero::Application::~Application()
{
    s_Instance = nullptr;
    for (auto& layer : m_LayerStack)
        layer->OnDetach();

    m_LayerStack.clear();
}

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

void aero::Application::Restart() { m_Running = false; }

void aero::Application::Run()
{
    m_Running = true;

    LOG_DEBUG_TAG("APPLICATION", "Starting the main application loop");

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
    }
    LOG_DEBUG_TAG("APPLICATION", "Stopping the main application loop");
}
