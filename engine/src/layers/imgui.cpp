#include "layers/imgui.hpp"
#include "version.hpp"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"

#include <print>

namespace echo {

PhaseState ImGuiLayer::onAttach() {
    // Imgui Initialization
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; 
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;    

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // High-DPI scaling
    ImGuiStyle& style = ImGui::GetStyle();
    float main_scale = SDL_GetDisplayContentScale(SDL_GetDisplayForWindow(m_window->getInternal()));
    //style.ScaleAllSizes(main_scale);       
    //style.FontScaleDpi = main_scale;
    
    int display_w, display_h;
    SDL_GetWindowSizeInPixels(m_window->getInternal(), &display_w, &display_h);
    io.DisplaySize = ImVec2((float)display_w * main_scale, (float)display_h * main_scale);
    io.DisplayFramebufferScale = ImVec2(1.0f / main_scale, 1.0f / main_scale);

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForSDLGPU(m_window->getInternal());
    ImGui_ImplSDLGPU3_InitInfo init_info = {};
    init_info.Device = m_gpuDevice->getInternal();
    init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(m_gpuDevice->getInternal(), m_window->getInternal());
    init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;                      
    init_info.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;  
    init_info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;
    ImGui_ImplSDLGPU3_Init(&init_info);

	return PhaseState::Success;
}

PhaseState ImGuiLayer::onPrepareFrame() {
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport(0, NULL, ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::BeginMainMenuBar();
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Exit")) {
            return PhaseState::Failure;
        }
        ImGui::EndMenu();
    }

    static bool show_demo_window = false;
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);
    if (ImGui::MenuItem("Demo")) {
        show_demo_window = !show_demo_window;
    }

    static bool show_about = false;
    if (ImGui::MenuItem("About")) {
        show_about = !show_about;
    }

    if (show_about) {
        ImGui::Begin("About", &show_about);
        ImGui::Text("%s", version.name.data());
        ImGui::Text("Version: %s", version.version.data());
        ImGui::Text("Build Type: %s", version.build_type.data());
        ImGui::End();
    }
    ImGui::EndMainMenuBar();

    return PhaseState::Continue;
}

PhaseState ImGuiLayer::onRenderFrame() {
    ImGui::Render(); // Bad naming, prepares draw data

    // Aquire Command Buffer
    SDL_GPUCommandBuffer* cmdbuf = SDL_AcquireGPUCommandBuffer(m_gpuDevice->getInternal());
    if (cmdbuf == NULL)
    {
        std::println("AcquireGPUCommandBuffer failed: %s", SDL_GetError());
        return PhaseState::Failure;
    }

    // Aquire ImGui Draw Data
    ImDrawData* draw_data = ImGui::GetDrawData();
    ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, cmdbuf);

    // Acquire Swapchain Texture
    SDL_GPUTexture* swapchainTexture;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdbuf, m_window->getInternal(), &swapchainTexture, NULL, NULL)) {
        std::println("WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
        return PhaseState::Failure;
    }

    // TODO: Remove clear so it gets drawn above other content
    // Clear Swapchain Texture
    if (swapchainTexture != NULL)
    {
        SDL_GPUColorTargetInfo colorTargetInfo = { 0 };
        colorTargetInfo.texture = swapchainTexture;
        colorTargetInfo.clear_color = (SDL_FColor){ 0.3f, 0.6f, 0.5f, 1.0f };
        colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
        colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
        colorTargetInfo.mip_level = 0;
        colorTargetInfo.layer_or_depth_plane = 0;
        colorTargetInfo.cycle = false;

        SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(cmdbuf, &colorTargetInfo, 1, NULL);
        
        // Render ImGui
        ImGui_ImplSDLGPU3_RenderDrawData(draw_data, cmdbuf, renderPass);
        
        SDL_EndGPURenderPass(renderPass);
    }

    // Submit command buffer
    SDL_SubmitGPUCommandBuffer(cmdbuf);

    return PhaseState::Continue;
}

EventState ImGuiLayer::onEvent(SDL_Event* event) {
    ImGui_ImplSDL3_ProcessEvent(event);
    return EventState::Propagate;
}

PhaseState ImGuiLayer::onDetach() {    
    SDL_WaitForGPUIdle(m_gpuDevice->getInternal());
    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui::DestroyContext();

    return PhaseState::Success;
}

} // namespace cinder