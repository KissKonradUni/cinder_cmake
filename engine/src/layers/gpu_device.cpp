#include "layers/gpu_device.hpp"
#include "version.hpp"

#include <print>

namespace prism {

PhaseState GPUDeviceLayer::onAttach() {
    m_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, version.build_type == "Debug", "vulkan");
    if (m_device == NULL) {
        std::println("Couldn't create GPU device: {}", SDL_GetError());
        return PhaseState::Failure;
    }

    if (!SDL_ClaimWindowForGPUDevice(m_device, m_window->getInternal()))
    {
        std::println("ClaimWindow failed");
        return PhaseState::Failure;
    }
    SDL_SetGPUSwapchainParameters(m_device, m_window->getInternal(), SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);

    return PhaseState::Success;
}

PhaseState GPUDeviceLayer::onDetach() {
    SDL_ReleaseWindowFromGPUDevice(m_device, m_window->getInternal());
    SDL_DestroyGPUDevice(m_device);
    return PhaseState::Success;
}

PhaseState GPUDeviceLayer::onRenderFrame(SDL_GPUCommandBuffer** commandBuffer, SDL_GPUTexture** swapchainTexture) {
    // Aquire command buffer
    auto commandBufferPtr = SDL_AcquireGPUCommandBuffer(m_device);
    if (!commandBufferPtr) {
        echo::logWarning(std::format("Failed to acquire GPU command buffer: {}", SDL_GetError()));
        return PhaseState::Failure;
    }
    *commandBuffer = commandBufferPtr;

    // Aquire swapchain texture
    SDL_GPUTexture* swapchainTexturePtr = nullptr;
    auto result = SDL_AcquireGPUSwapchainTexture(commandBufferPtr, m_window->getInternal(), &swapchainTexturePtr, NULL, NULL);
    if (!swapchainTexturePtr || result == false) {
        static bool hasLoggedSwapchainError = false;
        if (!hasLoggedSwapchainError) {
            echo::logWarning(std::format("Failed to acquire swapchain texture: {}", (result == false) ? SDL_GetError() : "NULL texture returned"));
            hasLoggedSwapchainError = true;
            echo::logWarning("This warning will only be logged once to avoid spamming the console.");
        }
        SDL_CancelGPUCommandBuffer(commandBufferPtr);
        *commandBuffer = nullptr;
        
        *swapchainTexture = nullptr;
        return PhaseState::Failure;
    }
    *swapchainTexture = swapchainTexturePtr;

    return PhaseState::Continue;
}

} // namespace prism