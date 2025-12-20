#pragma once

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_events.h>

namespace cinder {

class Host; // SDL wrapper

enum class PhaseState: uint8_t {
    Faliure = SDL_APP_FAILURE,
    Continue = SDL_APP_CONTINUE,
    Success = SDL_APP_SUCCESS,
};

enum class EventState: uint8_t {
    Continue = SDL_APP_CONTINUE,
    Consume = SDL_APP_SUCCESS,
};

class Layer {
public:
    friend class Host;

    virtual ~Layer() = default;
protected:
    virtual PhaseState onAttach() { return PhaseState::Success; }
    virtual PhaseState onDetach() { return PhaseState::Success; }
    virtual EventState onEvent(SDL_Event*) { return EventState::Continue; }
    
    virtual PhaseState onUpdate(float) { return PhaseState::Continue; }
    virtual PhaseState onPrepareFrame() { return PhaseState::Continue; }
    virtual PhaseState onRenderFrame() { return PhaseState::Continue; }

    Host* m_Host = nullptr;
};

} // namespace cinder