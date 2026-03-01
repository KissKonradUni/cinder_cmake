#pragma once

#include "../layer.hpp"
#include "window.hpp"
#include "gpu_device.hpp"

#include <filesystem>

namespace echo {

using namespace cinder;
using namespace prism;

using namespace std::filesystem;

class ImGuiLayer: public Layer {
public:
    ImGuiLayer(WindowLayer* window, GPUDeviceLayer* gpuDevice, path fontPath = "") : 
        m_window(window), m_gpuDevice(gpuDevice), m_fontPath(fontPath) {}
    ~ImGuiLayer() override = default;

protected:
    PhaseState onAttach() override;
    PhaseState onDetach() override;
    EventState onEvent(SDL_Event* event) override;
    PhaseState onPrepareFrame() override;
    PhaseState onRenderFrame(SDL_GPUCommandBuffer** commandBuffer, SDL_GPUTexture** swapchainTexture) override;

    WindowLayer* m_window;
    GPUDeviceLayer* m_gpuDevice;
    path m_fontPath;

    bool m_frameInFlight = false;
};

} // namespace echo