#include "world/component.hpp"

#include <print>

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

// UnknownComponent methods

std::span<uint8_t> UnknownComponent::getData() {
    return std::span<uint8_t>(data.data(), data.size());
}

void UnknownComponent::setData(const std::span<uint8_t>& newData) {
    if (newData.size() != data.size()) {
        std::println("UnknownComponent: New data size {} does not match existing size {}", newData.size(), data.size());
        throw std::runtime_error("UnknownComponent: Data size mismatch");
        return;
    }
    std::copy(newData.begin(), newData.end(), data.begin());
}

// ComponentPool methods
uint32_t ComponentPool::add(const std::span<uint8_t>& componentData) {
    if (componentData.size() != componentSize) {
        std::println("ComponentPool: Component data size {} does not match expected size {}", componentData.size(), componentSize);
        throw std::runtime_error("ComponentPool: Component data size mismatch");
        return UINT32_MAX;
    }

    if (!free.empty()) {
        uint32_t row = free.back();
        free.pop_back();
        data[row].setData(componentData);
        return row;
    } else {
        data.emplace_back(componentSize);
        data.back().setData(componentData);
        return static_cast<uint32_t>(data.size() - 1);
    }
}

void ComponentPool::remove(uint32_t row) {
    if (row >= data.size()) {
        std::println("ComponentPool: Row {} is out of bounds (max {})", row, data.size());
        // Non-critical error, just log
        return;
    }
    // Simply invalidate and add to free list
    free.push_back(row);
}

} // namespace hex