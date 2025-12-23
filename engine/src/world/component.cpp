#include "world/component.hpp"

#include <cstring>
#include <print>

namespace hex {

std::optional<ComponentTypeID> ComponentRegistry::registerComponent(const ComponentDescriptor& desc) {
    if (m_nameLookup.find(desc.stableName) != m_nameLookup.end()) {
        std::println("ComponentRegistry: Component with name {} is already registered!", desc.stableName);
        return std::nullopt;
    }
    
    ComponentTypeID typeID = m_nextTypeID++;
    m_nameLookup[desc.stableName] = typeID;
    m_descriptors[typeID] = desc;
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

std::optional<const ComponentDescriptor*> ComponentRegistry::getDescriptor(ComponentTypeID typeID) const {
    auto it = m_descriptors.find(typeID);
    if (it != m_descriptors.end()) {
        return &it->second;
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
    return m_descriptors.find(typeID) != m_descriptors.end();
}

// ComponentPool methods

ComponentPool::~ComponentPool() {
    deallocateBuffer();
}

uint32_t ComponentPool::allocate() {
    // Reuse from free list if possible
    if (!m_free.empty()) {
        uint32_t row = m_free.back();
        m_free.pop_back();
        return row;
    }

    // Reallocate the pool if we are at capacity
    reallocateBuffer(m_componentCount + COMPONENT_ALLOCATION_CHUNK_SIZE);

    auto id = m_componentCount++;
    // Call constructor if provided
    if (m_descriptor.constructor) {
        m_descriptor.constructor(&m_data[id * m_descriptor.size]);
    }

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

void ComponentPool::allocateBuffer(uint32_t newCapacity) {
    // C-style baby
    m_data = (uint8_t*)aligned_alloc(m_descriptor.alignment, m_descriptor.size * newCapacity);
    m_bufferCapacity = newCapacity;
}

void ComponentPool::reallocateBuffer(uint32_t newCapacity) {
    if (newCapacity <= m_bufferCapacity) {
        return; // No need to reallocate
    }

    uint8_t* newData = (uint8_t*)aligned_alloc(m_descriptor.alignment, m_descriptor.size * newCapacity);
    // Copy existing data
    memcpy(newData, m_data, m_descriptor.size * m_componentCount);
    // Free old data
    deallocateBuffer();

    // Update pointer and capacity
    m_data = newData;
    m_bufferCapacity = newCapacity;
}

void ComponentPool::deallocateBuffer() {
    // Call destructors for all allocated components
    if (m_descriptor.destructor) {
        for (uint32_t i = 0; i < m_componentCount; ++i) {
            // Skip free components
            if (std::find(m_free.begin(), m_free.end(), i) != m_free.end()) {
                continue;
            }
            m_descriptor.destructor(&m_data[i * m_descriptor.size]);
        }
    }
    
    free(m_data);
    m_data = nullptr;
    m_bufferCapacity = 0;
}

} // namespace hex