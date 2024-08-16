#ifndef _AERO_CORE_ENTRYPOINT_H_
#define _AERO_CORE_ENTRYPOINT_H_

#include <iostream>
#include "aero/core/application.h"
#include "aero/core/log.h"

extern aero::Application* aero::CreateApplication(int argc, char** argv);
bool g_ApplicationRunning = true;

namespace mps
{

int Main(int argc, char **argv)
{

    while (g_ApplicationRunning)
    {
        aero::Application* app = aero::CreateApplication(argc, argv);
        if (!app)
            break;
        app->Run();
        delete(app);
    }

    return 0;
}

} // namespace mps

#if 0 // defined(PLATFORM_WINDOWS) && !defined(HEADLESS) If I ever want to turn this into a gui application

#include <Windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
{
    return mps::Main(__argc, __argv);
}

#else

int main(int argc, char **argv) { return mps::Main(argc, argv); }

#endif // PLATFORM_WINDOWS

#endif // _AERO_CORE_ENTRYPOINT_H_
