#include "world/world.hpp"

#include <print>

namespace hex {

World::World(ComponentRegistry& componentRegistry)
	: m_entities(), m_freeIndices(), m_componentPools(),
	  m_componentRegistry(componentRegistry) {
	m_entities.reserve(ALLOCATION_CHUNK_SIZE);
}

World::~World() {}

Entity World::createEntity() {
	uint32_t index;

	if (!m_freeIndices.empty()) {
		index = m_freeIndices.back();
		m_freeIndices.pop_back();
	} else {
		index = static_cast<uint32_t>(m_entities.size());
		m_entities.emplace_back();
	}

	EntityRecord& record = m_entities[index];
	Entity entity{.id = index, .generation = record.generation};

	return entity;
}

void World::destroyEntity(const Entity entity) {
	if (!isEntityValid(entity)) {
		return;
	}

    // Remove all components associated with this entity
    for (auto& compRecord : m_entities[entity.id].components) {
        ComponentPool& pool = *m_componentPools[compRecord.type];
        pool.remove(compRecord.row);
    }

	EntityRecord& record = m_entities[entity.id];
	record.componentBits.clear();
	record.components.clear();
	record.generation += 1;

    m_freeIndices.push_back(entity.id);
};

bool World::isEntityValid(const Entity entity) const {
	if (entity.id >= this->m_entities.size()) {
		std::println("Entity ID {} is out of bounds (max {})", entity.id,
		             this->m_entities.size());
		return false;
	}

	const EntityRecord& record = this->m_entities[entity.id];
	return record.generation == entity.generation;
};

void World::addComponents(Entity entity, const std::vector<ComponentTypeID>& componentTypes) {
    if (!isEntityValid(entity)) {
        std::println("World: Cannot add components to invalid entity ID {}", entity.id);
        return;
    }

    EntityRecord& record = m_entities[entity.id];
    for (const auto& typeID : componentTypes) {
        auto componentSize = m_componentRegistry.getSize(typeID);
        if (!componentSize.has_value()) {
            std::println("World: Cannot add component of unknown type ID {} to entity ID {}", typeID, entity.id);
            continue;
        }

        // Ensure component pool exists
        if (typeID >= m_componentPools.size()) {
            m_componentPools.resize(typeID + 1);
            m_componentPools[typeID].reset(new ComponentPool(componentSize.value()));
        }

        ComponentPool* pool = m_componentPools[typeID].get();
        uint32_t row = pool->allocate();

        record.componentBits.set(typeID);
        record.components.push_back(ComponentRecord{.type = typeID, .row = row});
    }
}

void World::removeComponents(Entity entity, const std::vector<ComponentTypeID>& componentTypes) {
    if (!isEntityValid(entity)) {
        std::println("World: Cannot remove components from invalid entity ID {}", entity.id);
        return;
    }

    EntityRecord& record = m_entities[entity.id];
    for (const auto& typeID : componentTypes) {
        if (!record.componentBits.test(typeID)) {
            std::println("World: Entity ID {} does not have component type ID {}, cannot remove", entity.id, typeID);
            continue;
        }

        // Find component record
        auto it = std::find_if(record.components.begin(), record.components.end(),
                               [typeID](const ComponentRecord& rec) { return rec.type == typeID; });
        if (it == record.components.end()) {
            std::println("World: Trying to remove component type ID {} from entity ID {}, but no record found", typeID, entity.id);
            continue;
        }

        // Remove from pool
        if (typeID < m_componentPools.size() && m_componentPools[typeID]) {
            ComponentPool& pool = *m_componentPools[typeID];
            pool.remove(it->row);
        } else {
            std::println("World: No component pool for type ID {}, cannot remove from entity ID {}", typeID, entity.id);
        }

        // Remove from entity record
        record.componentBits.remove(typeID);
        record.components.erase(it);
    }
}

std::optional<ComponentPool*> World::getReadWriteComponentList(ComponentTypeID typeID) {
    // Check if typeID is valid
    if (!m_componentRegistry.isRegistered(typeID))
    {
        std::println("World: Unknown component type ID {}", typeID);
        return std::nullopt;
    }

    // Check if pool exists
    if (typeID >= m_componentPools.size() || !m_componentPools[typeID]) {
        std::println("World: No component pool for type ID {}", typeID);
        return std::nullopt;
    } else {
        return m_componentPools[typeID].get();
    }
}

std::optional<const ComponentPool*> World::getReadOnlyComponentList(ComponentTypeID typeID) const {
    // Get non-const version and cast to const
    auto poolOpt = const_cast<World*>(this)->getReadWriteComponentList(typeID);
    if (poolOpt.has_value()) {
        return poolOpt.value();
    } else {
        return std::nullopt;
    }
}

// WorldView

std::optional<const ComponentPool*> WorldView::readRef(ComponentTypeID typeID) const {
    // Check that system only writes the component it has access to
    assert(m_descriptor.reads.test(typeID));
    auto componentList = m_world.getReadOnlyComponentList(typeID);
    return componentList;
}

std::optional<ComponentPool*> WorldView::readWriteRef(ComponentTypeID typeID) {
    // Check that system only writes the component it has access to
    assert(m_descriptor.writes.test(typeID));
    auto componentList = m_world.getReadWriteComponentList(typeID);
    return componentList;
}

} // namespace hex