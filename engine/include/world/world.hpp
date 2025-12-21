#pragma once

#include "world/archetype.hpp"
#include "world/entity.hpp"

#include <memory>
#include <vector>

namespace hex {

#define ALLOCATION_CHUNK_SIZE 1024

class World {
public:
    World(ComponentRegistry* componentRegistry);
    ~World();

    Entity createEntity();
    void destroyEntity(const Entity entity);
    bool isEntityValid(const Entity entity) const;

    Archetype* findOrCreateArchetype(const DynBitset& componentMask);
    void addComponents(Entity entity, const std::vector<ComponentTypeID>& componentTypes);
protected:
    void updateRecord(Entity entity, uint32_t row);

    std::vector<std::unique_ptr<Archetype>> m_archetypes;
    std::vector<EntityRecord> m_entities;
    std::vector<uint32_t> m_freeIndices;

    ComponentRegistry* m_componentRegistry;
};

} // namespace hex