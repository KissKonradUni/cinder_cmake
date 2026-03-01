#pragma once

#include "../layer.hpp"
#include "window.hpp"
#include "gpu_device.hpp"

namespace prism {

using namespace cinder;

using namespace std::filesystem;

class RendererLayer: public Layer {
public:
    RendererLayer(WindowLayer* window, GPUDeviceLayer* gpuDevice) : 
        m_window(window), m_gpuDevice(gpuDevice) {}
    ~RendererLayer() override = default;

protected:
    PhaseState onAttach() override;
    PhaseState onDetach() override;
    EventState onEvent(SDL_Event* event) override;
    PhaseState onPrepareFrame() override;
    PhaseState onRenderFrame(SDL_GPUCommandBuffer** commandBuffer, SDL_GPUTexture** swapchainTexture) override;
    
    void loadShader(const std::filesystem::path& shaderPath);
    void createQuad();
    void createPipeline();
    
    void loadAssets();
    
    WindowLayer* m_window;
    GPUDeviceLayer* m_gpuDevice;
};

} // namespace prism