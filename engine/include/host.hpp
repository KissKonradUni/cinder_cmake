#pragma once

#include "layer.hpp"

#include <memory>
#include <vector>

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

namespace cinder {

enum class HostPhase {
    Attach,
    Detach,
    Other,
    Update,
    PrepareFrame,
    RenderFrame
};

class Host {
public:
    Host() = default;
    ~Host() = default;

    int run(int argc, char* argv[]);
    void printDebugInfo();

    template<typename T, typename... Args>
    std::enable_if_t<std::is_base_of_v<Layer, T>, T*> pushLayer(Args&&... args) {
        auto layer = std::make_unique<T>(std::forward<Args>(args)...);
        layer->m_Host = this;
        m_layers.emplace_back(std::move(layer));
        return static_cast<T*>(m_layers.back().get());
    }
    void popLayer(Layer* layer);

    inline const HostPhase& getCurrentPhase() const { return m_currentPhase; }
protected:
    static SDL_AppResult SDLCALL onAttach(void** appstate, int argc, char* argv[]);
    static SDL_AppResult SDLCALL onUpdate(void* appstate);
    static SDL_AppResult SDLCALL onEvent(void* appstate, SDL_Event* event);
    static void SDLCALL onDetach(void* appstate, SDL_AppResult result);
    static int SDLCALL __sdl_entrypoint(int argc, char* argv[]);

    std::vector<std::unique_ptr<Layer>> m_layers;
    HostPhase m_currentPhase = HostPhase::Attach;
    static Host* __internal_appstate;
};

} // namespace cinder