#include "world/component.hpp"

#include <print>
#include <cstring>

namespace hex {

std::optional<ComponentTypeID> ComponentRegistry::registerComponent(const ComponentDescriptor& desc) {
    if (m_nameLookup.find(desc.stableName) != m_nameLookup.end()) {
        std::println("ComponentRegistry: Component with name {} is already registered!", desc.stableName);
        return std::nullopt;
    }
    
    ComponentTypeID typeID = m_nextTypeID++;
    m_nameLookup[desc.stableName] = typeID;
    m_sizes[typeID] = desc.size;
    return typeID;
}

std::optional<ComponentTypeID> ComponentRegistry::getID(std::string_view stableName) const {
    auto it = m_nameLookup.find(std::string(stableName));
    if (it != m_nameLookup.end()) {
        return it->second;
    }
    std::println("ComponentRegistry: Unknown component name {}", stableName);
    return std::nullopt;
}

std::optional<std::size_t> ComponentRegistry::getSize(ComponentTypeID typeID) const {
    auto it = m_sizes.find(typeID);
    if (it != m_sizes.end()) {
        return it->second;
    }
    std::println("ComponentRegistry: Unknown ComponentTypeID {}", typeID);
    return std::nullopt;
}

std::optional<std::string_view> ComponentRegistry::getName(ComponentTypeID typeID) const {
    for (const auto& pair : m_nameLookup) {
        if (pair.second == typeID) {
            return pair.first;
        }
    }
    std::println("ComponentRegistry: Unknown ComponentTypeID {}", typeID);
    return std::nullopt;
}

bool ComponentRegistry::isRegistered(ComponentTypeID typeID) const {
    return m_sizes.find(typeID) != m_sizes.end();
}

// ComponentPool methods

uint32_t ComponentPool::allocate() {
    // Reuse from free list if possible
    if (!m_free.empty()) {
        uint32_t row = m_free.back();
        m_free.pop_back();
        return row;
    }

    // Reallocate the pool if we are at capacity
    if (this->getComponentCapacity() == this->getComponentCount()) {
        m_data.reserve((m_componentCount + COMPONENT_ALLOCATION_CHUNK_SIZE) * m_componentSize);
    }

    auto id = m_componentCount++;
    m_data.resize(m_componentCount * m_componentSize);
    return id;
}

void ComponentPool::remove(uint32_t row) {
    if (row >= m_componentCount) {
        std::println("ComponentPool: Row {} is out of bounds (max {})", row, m_componentCount);
        // Non-critical error, just log
        return;
    }
    // Simply invalidate and add to free list
    m_free.push_back(row);
}

} // namespace hex