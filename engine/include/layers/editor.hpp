#pragma once

#include "../layer.hpp"
#include "world/entity.hpp"

namespace echo {

using namespace cinder;

class EditorLayer : public Layer {
public:
    EditorLayer() = default;
    ~EditorLayer() override = default;
protected:
    PhaseState onPrepareFrame() override;
    EventState onEvent(SDL_Event* event) override;

    hex::Entity m_selectedEntity = {UINT32_MAX, 0};
};

} // namespace echo