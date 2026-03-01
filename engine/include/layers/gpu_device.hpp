#pragma once

#include "host.hpp"
#include "window.hpp"
#include "../layer.hpp"

#include "SDL3/SDL_gpu.h"

#include <cassert>

namespace prism {

using namespace cinder;

class GPUDeviceLayer : public Layer {
public:
    GPUDeviceLayer(WindowLayer* window) : m_window(window) {}
    ~GPUDeviceLayer() override = default;

    inline constexpr SDL_GPUDevice* getInternal() const { 
        // Ensure called during RenderFrame phase only
        assert(m_host->getCurrentPhase() == HostPhase::RenderFrame || m_host->getCurrentPhase() == HostPhase::Attach || m_host->getCurrentPhase() == HostPhase::Detach);

        return m_device; 
    }
protected:
    PhaseState onAttach() override;
    PhaseState onDetach() override;
    PhaseState onRenderFrame(SDL_GPUCommandBuffer** commandBuffer, SDL_GPUTexture** swapchainTexture) override;

    WindowLayer* m_window;
    SDL_GPUDevice* m_device = nullptr;
};

} // namespace prism