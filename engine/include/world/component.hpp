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

struct IComponent {}; // Serves as nothing but a base type

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

private:
    std::unordered_map<std::string, ComponentTypeID> m_nameLookup;
    std::unordered_map<ComponentTypeID, size_t> m_sizes;
    ComponentTypeID m_nextTypeID;
};

struct UnknownComponent {
    UnknownComponent(std::size_t size): data(size) {}

    std::span<uint8_t> getData();
    void setData(const std::span<uint8_t>& newData);    
protected:
    std::vector<uint8_t> data;
};

// Templated functions get implemented here to avoid linker errors
// Also type erasure is used here to allow WASM defined components
struct ComponentPool {
    ComponentPool(std::size_t compSize): componentSize(compSize), data(), free() {}

    uint32_t add(const std::span<uint8_t>& componentData);
    void remove(uint32_t row);
    
    // You may use UnknownComponent if you don't know the type at compile time (e.g. for WASM components)
    template<typename compType>
    std::optional<compType*> get(uint32_t row) {
        if (row >= data.size()) {
            std::println("ComponentPool: Row {} is out of bounds (max {})", row, data.size());
            return std::nullopt;
        }
        auto dataRef = data[row].getData();
        return reinterpret_cast<compType*>(dataRef.data());
    }
    
    // You may use UnknownComponent if you don't know the type at compile time (e.g. for WASM components)
    template<typename compType>
    std::span<compType> getAll() {
        return std::span<compType>(reinterpret_cast<compType*>(data.data()), data.size());
    }
protected:
    const std::size_t componentSize;
    std::vector<UnknownComponent> data;
    std::vector<uint32_t> free;
};

};