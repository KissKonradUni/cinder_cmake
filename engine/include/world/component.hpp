#pragma once

#include <unordered_map>
#include <cstdint>
#include <string>

namespace hex {

using ComponentTypeID = uint32_t;

struct ComponentDescriptor {
    std::string stableName;   // e.g. "cinder.Transform"
    std::size_t size;
    std::size_t alignment;
};

struct IComponent {}; // Serves as nothing but a base type

class ComponentRegistry {
public:
    ComponentRegistry(): m_nameLookup(), m_sizes(), m_nextTypeID(0) {}

    ComponentTypeID registerComponent(const ComponentDescriptor& desc);
    ComponentTypeID getID(std::string_view stableName) const;
    std::size_t getSize(ComponentTypeID typeID) const;

private:
    std::unordered_map<std::string, ComponentTypeID> m_nameLookup;
    std::unordered_map<ComponentTypeID, size_t> m_sizes;
    ComponentTypeID m_nextTypeID;
};

};