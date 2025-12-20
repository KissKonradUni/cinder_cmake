#include "layers/window.hpp"
#include "version.hpp"

#include <print>

#include <SDL3/SDL.h>

namespace cinder {

PhaseState WindowLayer::onAttach() {
    // Allow wayland High-DPI
    SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "SDL_VIDEO_WAYLAND_SCALE_TO_DISPLAY", "1", true);

    // SDL Initialization
	SDL_SetAppMetadata(version.name.data(), version.version.data(),
	                   std::format("app.{}.runtime", version.name).c_str());

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::println("Couldn't initialize SDL: %s", SDL_GetError());
        return PhaseState::Failure;
    }

    // Window and Renderer Creation
    SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    this->m_window = SDL_CreateWindow(version.name.data(), 1280, 720, window_flags);
    if (this->m_window == NULL) {
        std::println("Couldn't create window/renderer: %s", SDL_GetError());
        return PhaseState::Failure;
    }
    SDL_SetWindowPosition(this->m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(this->m_window);

	return PhaseState::Success;
}

EventState WindowLayer::onEvent(SDL_Event* event) {    
    return EventState::Propagate;
}

PhaseState WindowLayer::onDetach() {
    SDL_DestroyWindow(this->m_window);
    return PhaseState::Success;
}

} // namespace cinder