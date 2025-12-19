#include "cinder.hpp"
#include "version.hpp"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"

#include <rfl.hpp>
#include <rfl/json.hpp>

#include <print>

namespace cinder {

// Hacky way to get appstate into SDL callbacks
Application* __internal_appstate = nullptr;

SDL_AppResult SDLCALL __sdl_init(void** appstate, int argc, char* argv[]) {
    *appstate = __internal_appstate;
    __internal_appstate = nullptr;

    Application* app = (Application*)*appstate;

    // Allow wayland High-DPI
    SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "SDL_VIDEO_WAYLAND_SCALE_TO_DISPLAY", "1", true);

    // SDL Initialization
	SDL_SetAppMetadata(version.name.data(), version.version.data(),
	                   std::format("app.{}.runtime", version.name).c_str());

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::println("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Window and Renderer Creation
    SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    app->window = SDL_CreateWindow(version.name.data(), 1280, 720, window_flags);
    if (app->window == NULL) {
        std::println("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetWindowPosition(app->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(app->window);

    // GPU API Initialization
    app->device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, version.build_type == "Debug", nullptr);
    if (app->device == NULL) {
        std::println("Couldn't create GPU device: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_ClaimWindowForGPUDevice(app->device, app->window))
    {
        std::println("ClaimWindow failed");
        return SDL_APP_FAILURE;
    }
    SDL_SetGPUSwapchainParameters(app->device, app->window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);

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
    float main_scale = SDL_GetDisplayContentScale(SDL_GetDisplayForWindow(app->window));
    //style.ScaleAllSizes(main_scale);       
    //style.FontScaleDpi = main_scale;
    
    int display_w, display_h;
    SDL_GetWindowSizeInPixels(app->window, &display_w, &display_h);
    io.DisplaySize = ImVec2((float)display_w * main_scale, (float)display_h * main_scale);
    io.DisplayFramebufferScale = ImVec2(1.0f / main_scale, 1.0f / main_scale);

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForSDLGPU(app->window);
    ImGui_ImplSDLGPU3_InitInfo init_info = {};
    init_info.Device = app->device;
    init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(app->device, app->window);
    init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;                      
    init_info.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;  
    init_info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;
    ImGui_ImplSDLGPU3_Init(&init_info);

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDLCALL __sdl_iterate(void* appstate) { 
    const Application* app = (Application*)appstate;

    // Init ImGui Frame
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport(0, NULL, ImGuiDockNodeFlags_PassthruCentralNode);

    static bool show_demo_window = true;
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    ImGui::Render();

    // Aquire Command Buffer
    SDL_GPUCommandBuffer* cmdbuf = SDL_AcquireGPUCommandBuffer(app->device);
    if (cmdbuf == NULL)
    {
        std::println("AcquireGPUCommandBuffer failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Aquire ImGui Draw Data
    ImDrawData* draw_data = ImGui::GetDrawData();
    ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, cmdbuf);

    // Acquire Swapchain Texture
    SDL_GPUTexture* swapchainTexture;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdbuf, app->window, &swapchainTexture, NULL, NULL)) {
        SDL_Log("WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // 1st render pass
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

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDLCALL __sdl_event(void* appstate, SDL_Event* event) {
	if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS; 
    }

    ImGui_ImplSDL3_ProcessEvent(event);
    
    return SDL_APP_CONTINUE;
}

void SDLCALL __sdl_quit(void* appstate, SDL_AppResult result) {
    const Application* app = (Application*)appstate;
    
    SDL_WaitForGPUIdle(app->device);
    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui::DestroyContext();

    SDL_ReleaseWindowFromGPUDevice(app->device, app->window);
    SDL_DestroyGPUDevice(app->device);
    SDL_DestroyWindow(app->window);
}

int SDLCALL __sdl_entrypoint(int argc, char* argv[]) {
	return SDL_EnterAppMainCallbacks(argc, argv, 
                                     __sdl_init, __sdl_iterate,
	                                 __sdl_event, __sdl_quit);
}

void Application::printDebugInfo() {
	const std::string versionJson =
		rfl::json::write(version, rfl::json::pretty);
	std::println("Version Info:\n{}", versionJson);
}

int Application::run(int argc, char* argv[]) {
    __internal_appstate = this;
	return SDL_RunApp(argc, argv, __sdl_entrypoint, NULL);
}

} // namespace cinder