#pragma once

#include "../layer.hpp"

namespace echo {

using namespace cinder;

class DevToolsLayer : public Layer {
public:
    DevToolsLayer() = default;
    ~DevToolsLayer() override = default;
protected:
    PhaseState onPrepareFrame() override;
    EventState onEvent(SDL_Event* event) override;

    bool m_showDevToolsWindow = true;
};

} // namespace echo