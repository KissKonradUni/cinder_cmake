#include "world/world.hpp"

#include <print>
#include <memory>

namespace hex {

World::World(ComponentRegistry* componentRegistry): m_entities(), m_archetypes(), m_freeIndices(), m_componentRegistry(componentRegistry) {
    m_entities.reserve(ALLOCATION_CHUNK_SIZE);
}

World::~World() {
}

Entity World::createEntity() {
    uint32_t index;

    if (!m_freeIndices.empty()) {
        index = m_freeIndices.back();
        m_freeIndices.pop_back();
    } else {
        index = static_cast<uint32_t>(m_entities.size());
        m_entities.push_back({});
    }

    EntityRecord& record = m_entities[index];

    Entity entity {
        .id = index,
        .generation = record.generation
    };

    record.archetype = nullptr; // No components yet
    record.row = 0;             // TODO: determine row in archetype storage

    return entity;
};

void World::destroyEntity(const Entity entity) {
    if (!isEntityValid(entity)) {
        return;
    }

    EntityRecord& record = this->m_entities[entity.id];
    auto affectedRow = record.archetype->removeRow(record.row);
    this->updateRecord(affectedRow.affectedEntity, affectedRow.affectedRow);

    record.archetype = nullptr;
    record.row = -1;            // N/A
    record.generation++;        // Invalidate existing entity references

    this->m_freeIndices.push_back(entity.id);
};

bool World::isEntityValid(const Entity entity) const {
    if (entity.id >= this->m_entities.size()) {
        std::println("Entity ID {} is out of bounds (max {})", entity.id, this->m_entities.size());
        return false;
    }

    const EntityRecord& record = this->m_entities[entity.id];
    return record.generation == entity.generation;
};

Archetype* World::findOrCreateArchetype(const DynBitset& componentMask) {
    for (const auto& archetype : m_archetypes) {
        if (archetype->mask == componentMask) {
            return archetype.get();
        }
    }

    std::vector<std::size_t> componentSizes;
    for (uint32_t i = 0; i < componentMask.bitLength(); ++i) {
        if (componentMask.test(i)) {
            const std::size_t compSize = m_componentRegistry->getSize(i);
            componentSizes.push_back(compSize);
        }
    }

    auto newArchetype = std::make_unique<Archetype>(componentMask, componentSizes);
    Archetype* ptr = newArchetype.get();
    m_archetypes.push_back(std::move(newArchetype));
    return ptr;
}

// It's the user's responsibility to initialize new components
void World::addComponents(Entity entity, const std::vector<ComponentTypeID>& componentTypes) {
    if (!isEntityValid(entity)) {
        std::println("Cannot add components to invalid entity ID {}", entity.id);
        return;
    }

    // Get current record
    EntityRecord& record = this->m_entities[entity.id];
    DynBitset newMask = record.archetype ? record.archetype->mask : DynBitset();
    Archetype* oldArchetype = record.archetype;

    // Update mask
    for (const auto& typeID : componentTypes) {
        newMask.set(typeID);
    }

    // Find or create new archetype
    Archetype* newArchetype = findOrCreateArchetype(newMask);

    // Migrate entity to new archetype
    auto componentData = record.archetype ?
        record.archetype->collectRawComponentData(record.row) :
        std::make_unique<std::vector<std::span<uint8_t>>>();

    // Reorder component data and append new component to match new archetype layout
    std::vector<std::span<uint8_t>> reorderedData;
    if (oldArchetype == nullptr) {
        for (auto& compType : componentTypes) {
            size_t compSize = m_componentRegistry->getSize(compType);
            std::vector<uint8_t> emptyData(compSize, 0); // zero-initialize new component
            reorderedData.push_back(std::span<uint8_t>(emptyData.data(), compSize));
        }
    } else {
        for (uint32_t i = 0; i < newArchetype->mask.bitLength(); ++i) {
            if (newArchetype->mask.test(i)) {
                // Check if old archetype had this component
                if (oldArchetype->mask.test(i)) {
                    // Find index in old archetype
                    auto it = oldArchetype->m_typeToColumn.find(i);
                    if (it != oldArchetype->m_typeToColumn.end()) {
                        size_t columnIndex = it->second;
                        reorderedData.push_back((*componentData)[columnIndex]);
                    } else {
                        std::println("Inconsistent state: component {} in new archetype but not found in old archetype mapping", i);
                    }
                } else {
                    // New component, zero-initialize
                    size_t compSize = m_componentRegistry->getSize(i);
                    std::vector<uint8_t> emptyData(compSize, 0);
                    reorderedData.push_back(std::span<uint8_t>(emptyData.data(), compSize));
                }
            }
        }
    }
    
    auto flattenedData = std::vector<uint8_t>();
    for (const auto& span : reorderedData) {
        flattenedData.insert(flattenedData.end(), span.begin(), span.end());
    }

    auto newID = newArchetype->pushEntityWithComponents(entity, flattenedData);

    // Remove from old archetype
    if (record.archetype) {
        auto affectedRow = record.archetype->removeRow(record.row);
        this->updateRecord(affectedRow.affectedEntity, affectedRow.affectedRow);
    }

    // Update record
    record.archetype = newArchetype;
    record.row = newID;
}

void World::updateRecord(Entity entity, uint32_t row) {
    EntityRecord& record = this->m_entities[entity.id];
    record.row = row;
}

} // namespace hex