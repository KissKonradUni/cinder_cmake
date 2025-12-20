#pragma once

#include "../layer.hpp"
#include "window.hpp"
#include "gpu_device.hpp"

namespace echo {

using namespace cinder;
using namespace prism;

class ImGuiLayer: public Layer {
public:
    ImGuiLayer(WindowLayer* window, GPUDeviceLayer* gpuDevice) : 
        m_window(window), m_gpuDevice(gpuDevice) {}
    ~ImGuiLayer() override = default;

protected:
    PhaseState onAttach() override;
    PhaseState onDetach() override;
    EventState onEvent(SDL_Event* event) override;
    PhaseState onPrepareFrame() override;
    PhaseState onRenderFrame() override;

    WindowLayer* m_window;
    GPUDeviceLayer* m_gpuDevice;
};

} // namespace echo