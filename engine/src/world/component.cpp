#include "world/component.hpp"

#include <print>

namespace hex {

ComponentTypeID ComponentRegistry::registerComponent(const ComponentDescriptor& desc) {
    ComponentTypeID typeID = m_nextTypeID++;
    m_nameLookup[desc.stableName] = typeID;
    m_sizes[typeID] = desc.size;
    return typeID;
}

ComponentTypeID ComponentRegistry::getID(std::string_view stableName) const {
    auto it = m_nameLookup.find(std::string(stableName));
    if (it != m_nameLookup.end()) {
        return it->second;
    }
    std::println("ComponentRegistry: Unknown component name {}", stableName);
    return UINT32_MAX; // Invalid ID
}

std::size_t ComponentRegistry::getSize(ComponentTypeID typeID) const {
    auto it = m_sizes.find(typeID);
    if (it != m_sizes.end()) {
        return it->second;
    }
    std::println("ComponentRegistry: Unknown ComponentTypeID {}", typeID);
    return -1; // Invalid size
}

} // namespace hex