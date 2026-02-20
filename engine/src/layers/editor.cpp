#include "layers/editor.hpp"
#include "data/str.hpp"
#include "host.hpp"

#include "imgui.h"

namespace echo {

void RenderComponent(const size_t entityID, const hex::EntityRecord& entity, const hex::ComponentRecord& component, hex::ComponentRegistry& registry, hex::World& world) {
    const auto descriptorOpt = registry.getDescriptor(component.type);
    if (!descriptorOpt.has_value()) {
        ImGui::Text("No field information available.");
        return;
    }
    const auto& descriptor = descriptorOpt.value();
    const auto& fields = descriptor->fieldDescriptors;
    if (descriptor->fieldDescriptors.size() == 0) {
        ImGui::Text("No fields available.");
        return;
    }
    
    auto compPoolOpt = world.getComponent<UnknownComponent>({static_cast<uint32_t>(entityID), entity.generation}, component.type);
    if (!compPoolOpt.has_value()) {
        ImGui::Text("Component pool not found.");
        return;
    }

    UnknownComponent* compPtr = compPoolOpt.value();
    if (compPtr == nullptr) {
        ImGui::Text("Component instance not found.");
        return;
    }

    ImGui::Separator();
    ImGui::Text("%s (%03i):", descriptor->stableName.c_str(), component.row);
    ImGui::Separator();
    ImGui::BeginTable("##component_table", 2, ImGuiTableFlags_SizingStretchSame);
    ImGui::TableSetupColumn("Field", 0, 0.33f);
    ImGui::TableSetupColumn("Value", 0, 0.66f);
    for (uint32_t i = 0; i < descriptor->fieldDescriptors.size(); i++) {
        const auto& name   = fields[i].name;
        const auto  type   = fields[i].type;
        void* offsetPtr = static_cast<void*>(
            static_cast<char*>(static_cast<void*>(compPtr)) + fields[i].offset
        );

        ImGui::PushID(name.c_str());
        ImGui::TableNextColumn();
        ImGui::Text("%s:", name.c_str());
        ImGui::TableNextColumn();

        // TODO: Boring, implement all types

        if (type == ComponentFieldType::Str) {
            cinder::str* strField = static_cast<cinder::str*>(offsetPtr);
            if (strField->data != nullptr) {
                ImGui::SetNextItemWidth(-1);
                ImGui::InputText("##string_field", strField->data, strField->capacity);
            } else {
                ImGui::Text("%s: <uninitialized>", name.c_str());
            }
        } else if (type == ComponentFieldType::Float32) {
            float* floatField = static_cast<float*>(offsetPtr);
            ImGui::SetNextItemWidth(-1);
            ImGui::InputFloat("##float_field", floatField);
        } else if (type == ComponentFieldType::Int32) {
            int* intField = static_cast<int*>(offsetPtr);
            offsetPtr = static_cast<void*>(
                static_cast<char*>(offsetPtr) + sizeof(int)
            );
            ImGui::SetNextItemWidth(-1);
            ImGui::InputInt("##int_field", intField);
        } else if (type == ComponentFieldType::Vec2) {
            vec2* vecField = static_cast<vec2*>(offsetPtr);
            offsetPtr = static_cast<void*>(
                static_cast<char*>(offsetPtr) + sizeof(vec2)
            );
            ImGui::SetNextItemWidth(-1);
            ImGui::InputFloat2("##vec2_field", &vecField->x);
        } else if (type == ComponentFieldType::Vec3) {
            vec3* vecField = static_cast<vec3*>(offsetPtr);
            offsetPtr = static_cast<void*>(
                static_cast<char*>(offsetPtr) + sizeof(vec3)
            );
            ImGui::SetNextItemWidth(-1);
            ImGui::InputFloat3("##vec3_field", &vecField->x);
        } else if (type == ComponentFieldType::Vec4) {
            vec4* vecField = static_cast<vec4*>(offsetPtr);
            offsetPtr = static_cast<void*>(
                static_cast<char*>(offsetPtr) + sizeof(vec4)
            );
            ImGui::SetNextItemWidth(-1);
            ImGui::InputFloat4("##vec4_field", &vecField->x);
        } else if (type == ComponentFieldType::Bool) {
            bool* boolField = static_cast<bool*>(offsetPtr);
            ImGui::Checkbox("##bool_field", boolField);
        } else if (type == ComponentFieldType::Mat3) {
            mat3* matField = static_cast<mat3*>(offsetPtr);
            offsetPtr = static_cast<void*>(
                static_cast<char*>(offsetPtr) + sizeof(mat3)
            );
            ImGui::BeginTable("##mat3_table", 1);
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);
            ImGui::InputFloat3("##mat3_row0", &matField->m00);
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);
            ImGui::InputFloat3("##mat3_row1", &matField->m10);
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);
            ImGui::InputFloat3("##mat3_row2", &matField->m20);
            ImGui::EndTable();
        } else if (type == ComponentFieldType::Mat4) {
            mat4* matField = static_cast<mat4*>(offsetPtr);
            offsetPtr = static_cast<void*>(
                static_cast<char*>(offsetPtr) + sizeof(mat4)
            );
            ImGui::BeginTable("##mat4_table", 1);
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);
            ImGui::InputFloat4("##mat4_row0", &matField->m00);
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);
            ImGui::InputFloat4("##mat4_row1", &matField->m10); 
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);
            ImGui::InputFloat4("##mat4_row2", &matField->m20); 
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);
            ImGui::InputFloat4("##mat4_row3", &matField->m30); 
            ImGui::EndTable();
        } else {
            ImGui::Text("<unsupported field type>");
        }
        ImGui::PopID();
    }
    ImGui::EndTable();
}

PhaseState EditorLayer::onPrepareFrame() {    
    ImGui::Begin("Entity List");

    auto& world = m_host->getWorld();
    auto& registry = m_host->getComponentRegistry();
    const auto& entities = world.getEntityRecords();

    static bool collapse = false;
    static char buffer[256];
    snprintf(buffer, sizeof(buffer), "Entities: %zu", entities.size());
    if (ImGui::Button(buffer, ImVec2(-1, 0))) {
        collapse = !collapse;
    }
    if (!collapse) {
        for (uint32_t i = 0; i < entities.size(); ++i) {
            const auto& entity = entities[i];
            ImGui::PushID(static_cast<int>(i));
            
            snprintf(buffer, sizeof(buffer), "Entity %02u@%02u", i, entity.generation);
            if (ImGui::Selectable(
                buffer,
                m_selectedEntity.id == i && m_selectedEntity.generation == entity.generation
            )) {
                m_selectedEntity = {i, entity.generation};
            }
            ImGui::PopID();
        }
    }

    ImGui::End();

    ImGui::Begin("Entity Inspector");
    if (m_selectedEntity.id != UINT32_MAX) {
        const auto& entityRecord = entities[m_selectedEntity.id];
        if (entityRecord.generation == m_selectedEntity.generation) {
            ImGui::Text("Entity %02u@%02u", m_selectedEntity.id, m_selectedEntity.generation);
            const auto& componentRecords = entityRecord.components;
            for (const auto& compRecord : componentRecords) {
                RenderComponent(m_selectedEntity.id, entityRecord, compRecord, registry, world);
            }
        }
    }
    ImGui::End();
    
    return PhaseState::Continue;
}

EventState EditorLayer::onEvent(SDL_Event* event) {
    return EventState::Propagate;
}

} // namespace echo