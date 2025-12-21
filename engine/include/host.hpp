#pragma once

#include "world/component.hpp"
#include "world/world.hpp"
#include "layer.hpp"

#include <memory>
#include <vector>

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

namespace cinder {

using namespace hex;

enum class HostPhase {
    Attach,
    Detach,
    Other,
    Update,
    PrepareFrame,
    RenderFrame
};

struct Time {
    float renderDelta = 0.0f;
    float updateDelta = 0.0f;
    
    double lastUpdateTime = 0.0f;
    double lastRenderTime = 0.0f;
    double totalTime = 0.0f;
};

class Host {
public:
    Host() = default;
    ~Host() = default;

    int run(int argc, char* argv[]);
    void close();
    void printDebugInfo();

    template<typename T, typename... Args>
    std::enable_if_t<std::is_base_of_v<Layer, T>, T*> pushLayer(Args&&... args) {
        auto layer = std::make_unique<T>(std::forward<Args>(args)...);
        layer->m_host = this;
        m_layers.emplace_back(std::move(layer));
        return static_cast<T*>(m_layers.back().get());
    }
    void popLayer(Layer* layer);
    
    inline const HostPhase& getCurrentPhase() const { return m_currentPhase; }
    inline const Time& getTime() const { return m_time; }
    
    inline World& getWorld() { return m_world; }
    inline const World& getWorld() const { return m_world; }

    inline ComponentRegistry& getComponentRegistry() { return m_componentRegistry; }
    inline const ComponentRegistry& getComponentRegistry() const { return m_componentRegistry; }
protected:
    static SDL_AppResult SDLCALL onAttach(void** appstate, int argc, char* argv[]);
    static SDL_AppResult SDLCALL onRender(void* appstate);
    static SDL_AppResult SDLCALL onUpdate(void* appstate);
    static SDL_AppResult SDLCALL onEvent(void* appstate, SDL_Event* event);
    static void SDLCALL onDetach(void* appstate, SDL_AppResult result);
    static int SDLCALL __sdl_entrypoint(int argc, char* argv[]);

    Time m_time;
    
    ComponentRegistry m_componentRegistry;
    World m_world{ m_componentRegistry }; 
    
    HostPhase m_currentPhase = HostPhase::Attach;

    std::vector<std::unique_ptr<Layer>> m_layers;
    
    static Host* __internal_appstate;
};

} // namespace cinder