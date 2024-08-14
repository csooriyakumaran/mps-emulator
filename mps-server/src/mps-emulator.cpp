#ifndef _MPS_EMULATOR_H_
#define _MPS_EMULATOR_H_

#include "aero/core/application.h"
#include "aero/core/entrypoint.h"

#include "server-layer.h"

aero::Application* aero::CreateApplication(int argc, char** argv)
{
    aero::AppSpec spec;
    spec.Name = "MPS Emulator";
    spec.SleepMilliseconds = 0;

    aero::Application* app = new aero::Application(spec);
    app->PushLayer<ServerLayer>();

    return app;
}


#endif // _MPS_EMULATOR_H_
