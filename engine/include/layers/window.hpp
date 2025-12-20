#pragma once

#include "../layer.hpp"

namespace cinder {

class WindowLayer : public Layer {
public:
    WindowLayer() = default;
    ~WindowLayer() override = default;
    
    inline constexpr SDL_Window* getInternal() const { return m_window; }
protected:
    PhaseState onAttach() override;
    PhaseState onDetach() override;
    EventState onEvent(SDL_Event* event) override;

    SDL_Window* m_window = nullptr;
};

};