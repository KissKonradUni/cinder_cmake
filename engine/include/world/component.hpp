#pragma once

#include <unordered_map>
#include <optional>
#include <cstdint>
#include <string>
#include <memory>
#include <vector>
#include <print>
#include <span>

namespace hex {

using ComponentTypeID = uint32_t;

using CompCtorFunc = void(*)(void* memory);
using CompDtorFunc = void(*)(void* memory);

struct ComponentDescriptor {
    std::string stableName;   // e.g. "cinder.Transform"
    std::size_t size;
    std::size_t alignment;
    CompCtorFunc constructor = nullptr;
    CompDtorFunc destructor = nullptr;
};

template<typename T>
void component_ctor(void* p) {
    std::construct_at(static_cast<T*>(p));
}

template<typename T>
void component_dtor(void* p) {
    std::destroy_at(static_cast<T*>(p));
}

template<typename T>
ComponentDescriptor quick_component_desc(std::string name) {
    static_assert(std::is_trivially_copyable_v<T>);
    return ComponentDescriptor{
        .stableName  = std::move(name),
        .size        = sizeof(T),
        .alignment   = alignof(T),
        .constructor = component_ctor<T>,
        .destructor  = component_dtor<T>
    };
}

struct WASMComponent {}; // TODO: Add WASM specific functionality later

class ComponentRegistry {
public:
    ComponentRegistry() = default;
    ~ComponentRegistry() = default;

    ComponentRegistry(const ComponentRegistry&) = delete;
    ComponentRegistry& operator=(const ComponentRegistry&) = delete;

    std::optional<ComponentTypeID> registerComponent(const ComponentDescriptor& desc);
    std::optional<ComponentTypeID> getID(std::string_view stableName) const;
    std::optional<const ComponentDescriptor*> getDescriptor(ComponentTypeID typeID) const;
    std::optional<std::string_view> getName(ComponentTypeID typeID) const;
    bool isRegistered(ComponentTypeID typeID) const;
private:
    std::unordered_map<std::string, ComponentTypeID> m_nameLookup;
    std::unordered_map<ComponentTypeID, ComponentDescriptor> m_descriptors;
    ComponentTypeID m_nextTypeID = 0;
};

// This is just a pointer to raw data, type erased
struct UnknownComponent {};

#define COMPONENT_ALLOCATION_CHUNK_SIZE 64

// Templated functions get implemented here to avoid linker errors
// Also type erasure is used here to allow WASM defined components
struct ComponentPool {
    ComponentPool(const ComponentDescriptor& desc): 
        m_data(nullptr),
        m_descriptor(desc),
        m_componentCount(0),
        m_bufferCapacity(0) 
    {
        allocateBuffer(COMPONENT_ALLOCATION_CHUNK_SIZE);
    }
    ~ComponentPool();

    uint32_t allocate();
    void remove(uint32_t row);
    
    // You may use UnknownComponent if you don't know the type at compile time (e.g. for WASM components)
    template<typename compType>
    std::optional<compType*> get(uint32_t row) {
        if (row >= m_componentCount) {
            std::println("ComponentPool: Row {} is out of bounds (max {})", row, m_componentCount);
            return std::nullopt;
        }
        return (compType*)&m_data[row * m_descriptor.size];
    }
    
    // You may use UnknownComponent if you don't know the type at compile time (e.g. for WASM components)
    template<typename compType>
    std::span<compType> getAll() {
        return std::span<compType>((compType*)m_data, m_componentCount);
    }
protected:
    void allocateBuffer(uint32_t newCapacity);
    void reallocateBuffer(uint32_t newCapacity);
    void deallocateBuffer();

    uint8_t* m_data;
    std::vector<uint32_t> m_free;
    const ComponentDescriptor& m_descriptor;
    
    uint32_t m_componentCount = 0;
    uint32_t m_bufferCapacity = 0;
};

};