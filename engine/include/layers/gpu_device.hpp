#pragma once

#include "window.hpp"
#include "../layer.hpp"

#include "SDL3/SDL_gpu.h"

namespace prism {

using namespace cinder;

class GPUDeviceLayer : public Layer {
public:
    GPUDeviceLayer(WindowLayer* window) : m_window(window) {}
    ~GPUDeviceLayer() override = default;

    inline constexpr SDL_GPUDevice* getInternal() const { return m_device; }
protected:
    PhaseState onAttach() override;
    PhaseState onDetach() override;

    WindowLayer* m_window;
    SDL_GPUDevice* m_device = nullptr;
};

} // namespace prism