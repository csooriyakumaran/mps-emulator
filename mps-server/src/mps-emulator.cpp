#include "aero/core/application.h"
#include "aero/core/entrypoint.h"

#include "server-layer.h"
#include "utils/args.h"

aero::Application* aero::CreateApplication(int argc, char** argv)
{
    args::Options opts = args::ParseAguments(argc, argv);
    if (opts.should_close)
        return nullptr;

    aero::AppSpec spec;
    spec.Name              = "MPS Emulator";
    spec.SleepMilliseconds = 2000;

    aero::Application* app = new aero::Application(spec);
    app->PushLayer<ServerLayer>(opts.port);

    return app;
}
