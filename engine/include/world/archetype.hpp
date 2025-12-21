#pragma once

#include "world/bitset.hpp"
#include "world/entity.hpp"
#include "world/component.hpp"

#include <cstdint>
#include <cstring>
#include <memory>
#include <print>

namespace hex {

/*
 * Archetype represents a unique combination of component types.
 * It stores entities that share the same set of components in a
 * by column storage format.
 *
 * The columns themselves are contiguous arrays of raw component data.
 * The columns are not contiguous with each other.
 * 
 * Type erasure is used to store component data as raw bytes.
 *
 * @note Templates are used to provide typed access to component columns.
 *       They must live in the header. 
 */

#define INITIAL_ROW_CAPACITY 32

struct ComponentColumn {
    ComponentColumn(std::size_t componentSize)
        : m_componentSize(componentSize), data(INITIAL_ROW_CAPACITY * componentSize, 0) {}

    std::vector<uint8_t> data; // raw data storage

    template<typename CompType>
    std::span<CompType> asTypedArray() {
        std::size_t count = data.size() / m_componentSize;
        if (m_componentSize != sizeof(CompType)) {
            // I don't like exceptions, with correct usage this should never happen
            std::println("Component size mismatch in Archetype::ComponentColumn::asTypedArray");
            return {};
        }
        return std::span<CompType>(reinterpret_cast<CompType*>(data.data()), count);
    }

    inline std::size_t componentSize() const { return m_componentSize; }
protected:
    std::size_t m_componentSize;
};

struct RemoveResult {
    uint32_t affectedRow;
    Entity affectedEntity;
};

struct Archetype {
friend class World;
public:
    Archetype(const DynBitset& mask, const std::vector<std::size_t>& componentSizes): mask(mask), componentCount(mask.bitCount()),
        m_entities(), m_typeToColumn() {
        for (auto comp: componentSizes) {
            m_columns.emplace_back(comp);
        }
    }

    template<typename CompType>
    std::span<CompType> getColumn(ComponentTypeID typeID) {
        auto it = m_typeToColumn.find(typeID);
        if (it == m_typeToColumn.end()) {
            return {};
        }
        return m_columns[it->second].asTypedArray<CompType>();
    }

    template<typename CompType>
    CompType* getComponent(ComponentTypeID typeID, uint32_t row) {
        auto column = getColumn<CompType>(typeID);
        if (column == nullptr || row >= column->size()) {
            std::println("Invalid column or row in Archetype::getComponent");
            return nullptr;
        }
        return &(*column)[row];
    }

    const DynBitset mask;
    const uint32_t componentCount;
protected:
    uint32_t pushEntityWithComponents(const Entity& entity, const std::span<uint8_t>& componentData);
    void pushComponent(ComponentTypeID typeID, const std::span<uint8_t>& data);
    std::unique_ptr<std::vector<std::span<uint8_t>>> collectRawComponentData(uint32_t row);

    /**
     * @brief Remove row from archetype storage
     * 
     * @param row 
     * @return uint32_t The last item's new row index, that needs
     *                  to be updated in the EntityRecord.
     */
    RemoveResult removeRow(uint32_t row);

    std::vector<Entity> m_entities;
    std::vector<ComponentColumn> m_columns;
    std::unordered_map<ComponentTypeID, uint32_t> m_typeToColumn;
};

} // namespace hex