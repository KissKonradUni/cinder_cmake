#include "host.hpp"
#include "version.hpp"

#include <rfl.hpp>
#include <rfl/json.hpp>

#include <print>

namespace cinder {

Host* Host::__internal_appstate = nullptr;

SDL_AppResult SDLCALL Host::onAttach(void** appstate, int argc, char* argv[]) {
    *appstate = Host::__internal_appstate;
    Host::__internal_appstate = nullptr;

    Host* host = (Host*)*appstate;

    for (auto& layer : host->m_layers) {
        layer->m_host = host;
        layer->onAttach();
    }

    host->m_currentPhase = HostPhase::Other;
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDLCALL Host::onUpdate(void* appstate) {
    Host* host = (Host*)appstate;
    host->m_currentPhase = HostPhase::Update;

    host->m_time.totalTime = SDL_GetTicks() / 1000.0;
    host->m_time.updateDelta = static_cast<float>(host->m_time.totalTime - host->m_time.lastUpdateTime);
    host->m_time.lastUpdateTime = host->m_time.totalTime;

    for (auto& layer : host->m_layers) {
        auto result = layer->onUpdate(); 
        if (result == PhaseState::Failure) {
            return SDL_APP_FAILURE;
        }
    }

    host->m_currentPhase = HostPhase::Other;
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDLCALL Host::onRender(void* appstate) { 
    Host* host = (Host*)appstate;

    // TODO: Move to other thread
    onUpdate(appstate);

    host->m_currentPhase = HostPhase::PrepareFrame;
    for (auto& layer : host->m_layers) {
        auto result = layer->onPrepareFrame();
        if (result == PhaseState::Failure) {
            return SDL_APP_FAILURE;
        }
    }

    host->m_time.totalTime = SDL_GetTicks() / 1000.0;
    host->m_time.renderDelta = static_cast<float>(host->m_time.totalTime - host->m_time.lastRenderTime);
    host->m_time.lastRenderTime = host->m_time.totalTime;

    host->m_currentPhase = HostPhase::RenderFrame;
    for (auto& layer : host->m_layers) {
        auto result = layer->onRenderFrame();
        if (result == PhaseState::Failure) {
            return SDL_APP_FAILURE;
        }
    }

    host->m_currentPhase = HostPhase::Other;
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDLCALL Host::onEvent(void* appstate, SDL_Event* event) {
    Host* host = (Host*)appstate;
    host->m_currentPhase = HostPhase::Other;

	if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS; 
    }

    for (auto& layer : host->m_layers) {
        EventState state = layer->onEvent(event);
        if (state == EventState::Consume) {
            break;
        }
    }
    
    host->m_currentPhase = HostPhase::Other;
    return SDL_APP_CONTINUE;
}

void SDLCALL Host::onDetach(void* appstate, SDL_AppResult result) {
    Host* host = (Host*)appstate;
    host->m_currentPhase = HostPhase::Detach;

    // Reverse order
    for (auto it = host->m_layers.rbegin(); it != host->m_layers.rend(); ++it) {
        (*it)->onDetach();
    }
}

int SDLCALL Host::__sdl_entrypoint(int argc, char* argv[]) {
	return SDL_EnterAppMainCallbacks(argc, argv, 
                                     Host::onAttach, Host::onRender,
	                                 Host::onEvent,  Host::onDetach);
}

void Host::printDebugInfo() {
	const std::string versionJson =
		rfl::json::write(version, rfl::json::pretty);
	std::println("Version Info:\n{}", versionJson);
}

int Host::run(int argc, char* argv[]) {
    Host::__internal_appstate = this;
	return SDL_RunApp(argc, argv, Host::__sdl_entrypoint, NULL);
}

void Host::close() {
    SDL_Event quitEvent;
    quitEvent.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&quitEvent);
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