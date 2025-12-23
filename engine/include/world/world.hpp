#pragma once

#include "world/entity.hpp"
#include "system/system_descriptor.hpp"

#include <cassert>
#include <memory>
#include <vector>

namespace hex {

#define ALLOCATION_CHUNK_SIZE 64

class World {
public:
    World(ComponentRegistry& componentRegistry);
    ~World();

    World(const World&) = delete;
    World& operator=(const World&) = delete;

    Entity createEntity();
    void destroyEntity(const Entity entity);
    bool isEntityValid(const Entity entity) const;

    inline const std::vector<EntityRecord>& getEntityRecords() const { return m_entities; }

    /**
     * @brief Get the Component object
     * @note Most of the time you should not use this directly, but mutate components via systems or higher-level abstractions
     * 
     * @tparam compType The class you want the component returned as, can be UnknownComponent for generic access
     * @param entity The ID of the entity
     * @param typeID The type ID of the component
     * @return std::optional<compType*> The component pointer, or nullopt if not found
     */
    template<typename compType>
    std::optional<compType*> getComponent(Entity entity, ComponentTypeID typeID) {
        if (!isEntityValid(entity)) {
            std::println("World: Cannot get component of invalid entity ID {}", entity.id);
            return std::nullopt;
        }

        const EntityRecord& record = m_entities[entity.id];
        if (!record.componentBits.test(typeID)) {
            std::println("World: Entity ID {} does not have component type ID {}, cannot get", entity.id, typeID);
            return std::nullopt;
        }

        // Find component record
        auto it = std::find_if(record.components.begin(), record.components.end(),
                               [typeID](const ComponentRecord& rec) { return rec.type == typeID; });
        if (it == record.components.end()) {
            std::println("World: Trying to get component type ID {} from entity ID {}, but no record found", typeID, entity.id);
            return std::nullopt;
        }

        // Get from pool
        if (typeID < m_componentPools.size() && m_componentPools[typeID]) {
            ComponentPool& pool = *m_componentPools[typeID];
            return pool.get<compType>(it->row);
        } else {
            std::println("World: No component pool for type ID {}, cannot get from entity ID {}", typeID, entity.id);
            return std::nullopt;
        }
    }

    void addComponents(Entity entity, const std::vector<ComponentTypeID>& componentTypes);
    void removeComponents(Entity entity, const std::vector<ComponentTypeID>& componentTypes);
    std::optional<const ComponentPool*> getReadOnlyComponentList(ComponentTypeID typeID) const;
    std::optional<ComponentPool*> getReadWriteComponentList(ComponentTypeID typeID);
protected:
    std::vector<EntityRecord>                   m_entities;        // Indexed by Entity.id
    std::vector<uint32_t>                       m_freeIndices;     // Reusable entity indices
    std::vector<std::unique_ptr<ComponentPool>> m_componentPools;  // Indexed by ComponentTypeID

    ComponentRegistry& m_componentRegistry;
};

class WorldView {
public:
    WorldView(World& w, const SystemDescriptor& d) : m_world(w), m_descriptor(d) {}

    std::optional<const ComponentPool*> readRef(ComponentTypeID typeID) const;
    std::optional<ComponentPool*> readWriteRef(ComponentTypeID typeID);

private:
    World& m_world;
    const SystemDescriptor& m_descriptor;
};

} // namespace hex