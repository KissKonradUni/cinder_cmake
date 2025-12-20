#pragma once

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_events.h>

namespace cinder {

class Host; // SDL wrapper

enum class PhaseState: uint8_t {
    Failure = SDL_APP_FAILURE,
    Continue = SDL_APP_CONTINUE,
    Success = SDL_APP_SUCCESS,
};

enum class EventState: uint8_t {
    Propagate = SDL_APP_CONTINUE,
    Consume = SDL_APP_SUCCESS,
};

class Layer {
public:
    friend class Host;

    virtual ~Layer() = default;
protected:
    virtual PhaseState onAttach() { return PhaseState::Success; }
    virtual PhaseState onDetach() { return PhaseState::Success; }
    virtual EventState onEvent(SDL_Event*) { return EventState::Propagate; }
    
    /**
     * @brief Separate update phase before frame rendering
     * @note onUpdate must be deterministic, side-effect free outside Host-owned state, 
     *       and must not touch GPU, SDL windowing, or ImGui.
     * 
     * @return PhaseState - Determines whether to continue or abort the main loop
     */
    virtual PhaseState onUpdate(float) { return PhaseState::Continue; }
    virtual PhaseState onPrepareFrame() { return PhaseState::Continue; }
    virtual PhaseState onRenderFrame() { return PhaseState::Continue; }

    Host* m_Host = nullptr;
};

} // namespace cinder