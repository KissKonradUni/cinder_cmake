#pragma once

#include "../layer.hpp"

namespace echo {

using namespace cinder;

class EditorLayer : public Layer {
public:
    EditorLayer() = default;
    ~EditorLayer() override = default;
protected:
    PhaseState onPrepareFrame() override;
    EventState onEvent(SDL_Event* event) override;
};

} // namespace echo