#pragma once

#include <unordered_map>
#include <optional>
#include <cstdint>
#include <string>
#include <vector>
#include <print>
#include <span>

namespace hex {

using ComponentTypeID = uint32_t;

struct ComponentDescriptor {
    std::string stableName;   // e.g. "cinder.Transform"
    std::size_t size;
    std::size_t alignment;
};

struct IWASMComponent {}; // TODO: Add WASM specific functionality later

class ComponentRegistry {
public:
    ComponentRegistry(): m_nameLookup(), m_sizes(), m_nextTypeID(0) {}
    ~ComponentRegistry() = default;

    ComponentRegistry(const ComponentRegistry&) = delete;
    ComponentRegistry& operator=(const ComponentRegistry&) = delete;

    std::optional<ComponentTypeID> registerComponent(const ComponentDescriptor& desc);
    std::optional<ComponentTypeID> getID(std::string_view stableName) const;
    std::optional<std::size_t> getSize(ComponentTypeID typeID) const;
    std::optional<std::string_view> getName(ComponentTypeID typeID) const;
    bool isRegistered(ComponentTypeID typeID) const;
private:
    std::unordered_map<std::string, ComponentTypeID> m_nameLookup;
    std::unordered_map<ComponentTypeID, size_t> m_sizes;
    ComponentTypeID m_nextTypeID;
};

// This is just a pointer to raw data, type erased
struct UnknownComponent {};

#define COMPONENT_ALLOCATION_CHUNK_SIZE 16

// Templated functions get implemented here to avoid linker errors
// Also type erasure is used here to allow WASM defined components
struct ComponentPool {
    ComponentPool(std::size_t compSize): m_componentSize(compSize), m_data(), m_free() {
        m_data.reserve(COMPONENT_ALLOCATION_CHUNK_SIZE * m_componentSize);
    }
    ~ComponentPool() = default;

    uint32_t allocate();
    void remove(uint32_t row);
    
    // You may use UnknownComponent if you don't know the type at compile time (e.g. for WASM components)
    template<typename compType>
    std::optional<compType*> get(uint32_t row) {
        if (row >= m_componentCount) {
            std::println("ComponentPool: Row {} is out of bounds (max {})", row, m_componentCount);
            return std::nullopt;
        }
        return (compType*)&m_data[row * m_componentSize];
    }
    
    // You may use UnknownComponent if you don't know the type at compile time (e.g. for WASM components)
    template<typename compType>
    std::span<compType> getAll() {
        return std::span<compType>((compType*)m_data.data(), m_componentCount);
    }
protected:
    inline std::size_t getComponentCount() const {
        return m_componentCount;
    }

    inline std::size_t getComponentCapacity() const {
        return m_data.capacity() / m_componentSize;
    }

    const std::size_t m_componentSize;
    std::vector<uint8_t> m_data;
    std::vector<uint32_t> m_free;
    uint32_t m_componentCount = 0;
};

};