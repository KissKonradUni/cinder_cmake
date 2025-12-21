#include "layers/devtools.hpp"
#include "host.hpp"

#include "imgui.h"

namespace echo {

PhaseState DevToolsLayer::onPrepareFrame() {
    if (!m_showDevToolsWindow)
        return PhaseState::Continue;
    
    ImGui::Begin("DevTools", &m_showDevToolsWindow);

    ImGui::Text("Timings: ");
    ImGui::Text("  Total Time: %.3f s", m_host->getTime().totalTime);
    ImGui::Text("  Update Delta: %.3f ms", m_host->getTime().updateDelta);
    ImGui::Text("  Render Delta: %.3f ms", m_host->getTime().renderDelta);

    ImGui::End();
    
    return PhaseState::Continue;
}

EventState DevToolsLayer::onEvent(SDL_Event* event) {
    if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.scancode == SDL_SCANCODE_F12) {
            m_showDevToolsWindow = !m_showDevToolsWindow;
            return EventState::Consume;
        }
    }

    return EventState::Propagate;
}

} // namespace echo