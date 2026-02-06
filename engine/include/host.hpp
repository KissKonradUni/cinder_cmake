#pragma once

#include "logging.hpp"
#include "system/system.hpp"
#include "system/worker.hpp"

#include "world/component.hpp"
#include "world/world.hpp"

#include "data/time.hpp"

#include "host_phase.hpp"
#include "layer.hpp"

#include <memory>
#include <vector>
#include <atomic>

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

namespace cinder {

using namespace hex;

class Host {
friend class PhaseGuard;
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
    
    inline const std::atomic<HostPhase>& getCurrentPhase() const { return m_currentPhase; }
    inline const Time& getTime() const { return m_time; }
    
    inline World& getWorld() { return m_world; }
    inline const World& getWorld() const { return m_world; }

    inline ComponentRegistry& getComponentRegistry() { return m_componentRegistry; }
    inline const ComponentRegistry& getComponentRegistry() const { return m_componentRegistry; }

    SystemID registerSystem(SystemDescriptor& descriptor, SystemFuncPtr func);
    void unregisterSystem(SystemID systemID);
protected:
    void runSystems(HostPhase phase);

    static SDL_AppResult SDLCALL onAttach(void** appstate, int argc, char* argv[]);
    static SDL_AppResult SDLCALL onRender(void* appstate);
    static SDL_AppResult SDLCALL onUpdate(void* appstate);
    static SDL_AppResult SDLCALL onEvent(void* appstate, SDL_Event* event);
    static void SDLCALL onDetach(void* appstate, SDL_AppResult result);
    static int SDLCALL __sdl_entrypoint(int argc, char* argv[]);

    Time m_time;

    echo::Logger m_logger;
    
    uint32_t m_nextSystemID = 0;
    std::unordered_map<SystemPhase, std::vector<std::unique_ptr<System>>> m_systems;
    ComponentRegistry m_componentRegistry;
    World m_world{ m_componentRegistry, &m_time }; 
    SystemWorkerPool m_systemWorkerPool;
    
    std::atomic<HostPhase> m_currentPhase = HostPhase::Attach;

    std::vector<std::unique_ptr<Layer>> m_layers;
    
    static Host* __internal_appstate;
};

class PhaseGuard {
public:
    PhaseGuard(Host& h, HostPhase p) : m_host(h) {
        m_prev = m_host.m_currentPhase.exchange(p, std::memory_order_acq_rel);
    }
    ~PhaseGuard() {
        m_host.m_currentPhase.store(m_prev, std::memory_order_release);
    }
private:
    Host& m_host;
    HostPhase m_prev;
};

} // namespace cinder