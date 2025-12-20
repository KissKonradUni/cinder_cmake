#include "layers/gpu_device.hpp"
#include "version.hpp"

#include <print>

namespace prism {

PhaseState GPUDeviceLayer::onAttach() {
    m_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, version.build_type == "Debug", nullptr);
    if (m_device == NULL) {
        std::println("Couldn't create GPU device: %s", SDL_GetError());
        return PhaseState::Faliure;
    }

    if (!SDL_ClaimWindowForGPUDevice(m_device, m_window->getInternal()))
    {
        std::println("ClaimWindow failed");
        return PhaseState::Faliure;
    }
    SDL_SetGPUSwapchainParameters(m_device, m_window->getInternal(), SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);

    return PhaseState::Success;
}

PhaseState GPUDeviceLayer::onDetach() {
    SDL_ReleaseWindowFromGPUDevice(m_device, m_window->getInternal());
    SDL_DestroyGPUDevice(m_device);
    return PhaseState::Success;
}

} // namespace prism