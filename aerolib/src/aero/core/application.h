#ifndef _AERO_CORE_APPLICATION_H_
#define _AERO_CORE_APPLICATION_H_

#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

#include "aero/core/layer.h"
#include "aero/core/timer.h"
namespace aero
{

struct AppSpec
{
    std::string Name = "Aero Application";
    uint64_t SleepMilliseconds = 0;
};

class Application
{
public:
    Application(const AppSpec& spec = AppSpec());
    ~Application();

    static Application& Get();

    void Run();
    void Restart();
    void Shutdown();

    template<typename T, typename...Args>
    void PushLayer(Args&&... args)
    {
        static_assert(std::is_base_of<Layer, T>::value, "Pushed layer is not subclass of Layer");
        m_LayerStack.emplace_back(std::make_shared<T>(std::forward<Args>(args)...))->OnAttach();
    }
    void PushLayer(const std::shared_ptr<Layer>& layer)
    {
        m_LayerStack.emplace_back(layer);
        layer->OnAttach();
    }

    float FrameRate() { return 1.0f / m_FrameTime; }

private:
    void Init();

private:
    AppSpec m_Specification;
    bool m_Running        = false;

    float m_FrameTime     = 0.0f;
    float m_LastFrameTime = 0.0f;
    float m_FrameRate     = 0.0f;

    Timer m_AppTimer;

    std::vector<std::shared_ptr<Layer>> m_LayerStack;
};

//- implemented by the client application
Application* CreateApplication(int argc, char** argv);

} // namespace aero

#endif // _AERO_CORE_APPLICATION_H_
