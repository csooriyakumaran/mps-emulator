#ifndef _AERO_CORE_LAYER_H_
#define _AERO_CORE_LAYER_H_
namespace aero
{

class Layer
{
public:
    virtual ~Layer() = default;

    virtual void OnAttach() {}
    virtual void OnDetach() {}

    virtual void OnUpdate() {}
};
}
#endif // _AERO_CORE_LAYER_H_
