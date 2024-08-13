#include <iostream>

#include "mps-emulator.h"

namespace mps
{

int Main(int argc, char **argv)
{

    App app = App();
    app.Run();

    return 0;
}

} // namespace mps

#if 0

#include <Windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
{
    return mps::Main(__argc, __argv);
}

#else

int main(int argc, char **argv) { return mps::Main(argc, argv); }

#endif // PLATFORM_WINDOWS
