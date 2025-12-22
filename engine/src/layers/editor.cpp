#include "layers/editor.hpp"
#include "host.hpp"

#include "imgui.h"

namespace echo {

struct EntityNameComponent {
    std::string name;
};

PhaseState EditorLayer::onPrepareFrame() {    
    ImGui::Begin("Scene");

    auto& world = m_host->getWorld();
    auto& registry = m_host->getComponentRegistry();
    const auto& entities = world.getEntityRecords();

    ImGui::Text("Entities (%zu):", entities.size());
    for (size_t i = 0; i < entities.size(); ++i) {
        const auto& entityRecord = entities[i];
        ImGui::PushID(static_cast<int>(i));
        if (ImGui::TreeNode(("Entity " + std::to_string(i)).c_str())) {
            ImGui::Text("Generation: %u", entityRecord.generation);
            ImGui::Text("Components:");
            for (const auto& compRecord : entityRecord.components) {
                if (ImGui::TreeNode(std::format("{} (Type ID {})", 
                                                registry.getName(compRecord.type).value_or("Unknown"), 
                                                compRecord.type).c_str())) {
                    // TODO: Magic number for demo purposes
                    if (compRecord.type == 0) { // EntityNameComponent
                        Entity entity{.id = static_cast<uint32_t>(i), .generation = entityRecord.generation};
                        auto nameCompOpt = world.getComponent<EntityNameComponent>(entity, compRecord.type);
                        if (nameCompOpt.has_value()) {
                            ImGui::Text("  Name: %s", (*nameCompOpt.value()).name.c_str());
                        }
                    }
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    ImGui::End();
    
    return PhaseState::Continue;
}

EventState EditorLayer::onEvent(SDL_Event* event) {
    return EventState::Propagate;
}

} // namespace echo