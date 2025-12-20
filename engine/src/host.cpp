#include "host.hpp"
#include "version.hpp"

#include <rfl.hpp>
#include <rfl/json.hpp>

#include <print>

namespace cinder {

// Hacky way to get appstate into SDL callbacks
static Host* __internal_appstate = nullptr;

SDL_AppResult SDLCALL Host::onAttach(void** appstate, int argc, char* argv[]) {
    *appstate = __internal_appstate;
    __internal_appstate = nullptr;

    Host* host = (Host*)*appstate;

    for (auto& layer : host->m_layers) {
        layer->m_Host = host;
        layer->onAttach();
    }

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDLCALL Host::onUpdate(void* appstate) { 
    const Host* host = (Host*)appstate;

    for (auto& layer : host->m_layers) {
        // TODO: delta time
        auto result = layer->onUpdate(0.166f); 
        if (result == PhaseState::Failure) {
            return SDL_APP_FAILURE;
        }
    }

    for (auto& layer : host->m_layers) {
        auto result = layer->onPrepareFrame();
        if (result == PhaseState::Failure) {
            return SDL_APP_FAILURE;
        }
    }

    for (auto& layer : host->m_layers) {
        auto result = layer->onRenderFrame();
        if (result == PhaseState::Failure) {
            return SDL_APP_FAILURE;
        }
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDLCALL Host::onEvent(void* appstate, SDL_Event* event) {
    const Host* host = (Host*)appstate;

	if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS; 
    }

    for (auto& layer : host->m_layers) {
        EventState state = layer->onEvent(event);
        if (state == EventState::Consume) {
            break;
        }
    }
    
    return SDL_APP_CONTINUE;
}

void SDLCALL Host::onDetach(void* appstate, SDL_AppResult result) {
    const Host* host = (Host*)appstate;

    // Reverse order
    for (auto it = host->m_layers.rbegin(); it != host->m_layers.rend(); ++it) {
        (*it)->onDetach();
    }
}

int SDLCALL __sdl_entrypoint(int argc, char* argv[]) {
	return SDL_EnterAppMainCallbacks(argc, argv, 
                                     Host::onAttach , Host::onUpdate,
	                                 Host::onEvent, Host::onDetach);
}

void Host::printDebugInfo() {
	const std::string versionJson =
		rfl::json::write(version, rfl::json::pretty);
	std::println("Version Info:\n{}", versionJson);
}

int Host::run(int argc, char* argv[]) {
    __internal_appstate = this;
	return SDL_RunApp(argc, argv, __sdl_entrypoint, NULL);
}

void Host::popLayer(Layer* layer) {
    auto it = std::find_if(m_layers.begin(), m_layers.end(),
                           [layer](const std::unique_ptr<Layer>& ptr) { return ptr.get() == layer; });
    if (it != m_layers.end()) {
        (*it)->onDetach();
        m_layers.erase(it);
    }
}

} // namespace cinder