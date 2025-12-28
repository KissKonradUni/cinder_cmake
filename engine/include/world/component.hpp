#pragma once

#include "data/math.hpp"
#include "data/str.hpp"
#include <unordered_map>
#include <optional>
#include <cstdint>
#include <string>
#include <memory>
#include <vector>
#include <print>
#include <span>
#include <map>

#include  <rfl/fields.hpp>

namespace hex {

using ComponentTypeID = uint32_t;

using CompCtorFunc = void(*)(void* memory);
using CompDtorFunc = void(*)(void* memory);

enum class ComponentFieldType: uint8_t {
    Unknown,
    Float32,
    Float64,
    UInt8,
    UInt16,
    UInt32,
    UInt64,
    Int8,
    Int16,
    Int32,
    Int64,
    Bool,
    Str,
    Vec2,
    Vec3,
    Vec4,
    Mat3,
    Mat4,
};

// Wohoo generated garbage
template<typename T>
constexpr ComponentFieldType as_field_enum() {
    if constexpr (std::is_same_v<T, float>) {
        return ComponentFieldType::Float32;
    } else if constexpr (std::is_same_v<T, double>) {
        return ComponentFieldType::Float64;
    } else if constexpr (std::is_same_v<T, uint8_t>) {
        return ComponentFieldType::UInt8;
    } else if constexpr (std::is_same_v<T, uint16_t>) {
        return ComponentFieldType::UInt16;
    } else if constexpr (std::is_same_v<T, uint32_t>) {
        return ComponentFieldType::UInt32;
    } else if constexpr (std::is_same_v<T, uint64_t>) {
        return ComponentFieldType::UInt64;
    } else if constexpr (std::is_same_v<T, int8_t>) {
        return ComponentFieldType::Int8;
    } else if constexpr (std::is_same_v<T, int16_t>) {
        return ComponentFieldType::Int16;
    } else if constexpr (std::is_same_v<T, int32_t>) {
        return ComponentFieldType::Int32;
    } else if constexpr (std::is_same_v<T, int64_t>) {
        return ComponentFieldType::Int64;
    } else if constexpr (std::is_same_v<T, bool>) {
        return ComponentFieldType::Bool;
    } else if constexpr (std::is_same_v<T, cinder::str>) {
        return ComponentFieldType::Str;
    } else if constexpr (std::is_same_v<T, vec2>) {
        return ComponentFieldType::Vec2;
    } else if constexpr (std::is_same_v<T, vec3>) {
        return ComponentFieldType::Vec3;
    } else if constexpr (std::is_same_v<T, vec4>) {
        return ComponentFieldType::Vec4;
    } else if constexpr (std::is_same_v<T, mat3>) {
        return ComponentFieldType::Mat3;
    } else if constexpr (std::is_same_v<T, mat4>) {
        return ComponentFieldType::Mat4;
    } else if constexpr (std::is_same_v<T, quaternion>) {
        return ComponentFieldType::Vec4; // Quaternions are stored as vec4
    } else {
        return ComponentFieldType::Unknown;
    }
}

struct FieldDescriptor {
    std::string name;
    ComponentFieldType type;
    uint32_t offset;
    uint32_t size;    
};

struct ComponentDescriptor {
    std::string stableName;   // e.g. "cinder.Transform"
    
    std::size_t size;
    std::size_t alignment;
    
    std::vector<FieldDescriptor> fieldDescriptors;

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
ComponentDescriptor quick_component_desc(std::string name, std::vector<FieldDescriptor> fields) {
    //static_assert(std::is_trivially_copyable_v<T>);
    return ComponentDescriptor{
        .stableName       = std::move(name),
        .size             = sizeof(T),
        .alignment        = alignof(T),
        .fieldDescriptors = std::move(fields),
        .constructor      = component_ctor<T>,
        .destructor       = component_dtor<T>
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