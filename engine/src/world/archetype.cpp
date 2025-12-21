#include "world/archetype.hpp"

namespace hex {

uint32_t Archetype::pushEntityWithComponents(
	const Entity& entity, const std::span<uint8_t>& componentData) {
	m_entities.push_back(entity);
	size_t offset = 0;

	auto correctSize = m_columns[0].componentSize();
	for (int i = 1; i < m_columns.size(); ++i) {
		correctSize += m_columns[i].componentSize();
	}

	if (componentData.size() != correctSize) {
		std::println("Component data size mismatch in "
		             "Archetype::pushEntityWithComponents");
		return UINT32_MAX;
	}

	for (auto& [typeID, columnIndex] : m_typeToColumn) {
		auto& column = m_columns[columnIndex];
		std::span<uint8_t> compSpan(componentData.data() + offset,
		                            column.componentSize());
		pushComponent(typeID, compSpan);
		offset += column.componentSize();
	}

    return static_cast<uint32_t>(m_entities.size() - 1);
}

void Archetype::pushComponent(ComponentTypeID typeID,
                              const std::span<uint8_t>& data) {
	auto& column = m_columns[m_typeToColumn[typeID]];
	size_t oldSize = column.data.size();
	size_t needed = oldSize + column.componentSize();
	if (column.data.capacity() < needed) {
		column.data.reserve(needed + INITIAL_ROW_CAPACITY); // reserve more for future
	}
	column.data.resize(needed);
	std::memcpy(column.data.data() + oldSize, data.data(),
	            column.componentSize());
}

std::unique_ptr<std::vector<std::span<uint8_t>>>
Archetype::collectRawComponentData(uint32_t row) {
	auto buffer = std::make_unique<std::vector<std::span<uint8_t>>>();
	for (auto& [typeID, columnIndex] : m_typeToColumn) {
		auto& column = m_columns[columnIndex];
		size_t compSize = column.componentSize();
		size_t offset = row * compSize;
		std::span<uint8_t> compSpan(column.data.data() + offset, compSize);
        buffer->push_back(compSpan);
	}
	return buffer;
}

RemoveResult Archetype::removeRow(uint32_t row) {
    if (row >= m_entities.size()) {
        std::println("Invalid row index in Archetype::removeRow");
        return {UINT32_MAX, UINT32_MAX};
    }

    // Get last row
    uint32_t lastRow = static_cast<uint32_t>(m_entities.size() - 1);
    m_entities[row] = m_entities[lastRow];
    m_entities.pop_back();

    // Remove from each column
    for (auto& column : m_columns) {
        size_t compSize = column.componentSize();
        size_t lastOffset = lastRow * compSize;
        size_t rowOffset = row * compSize;
        std::memcpy(column.data.data() + rowOffset,
                    column.data.data() + lastOffset,
                    compSize);
        // We don't shrink the vector to avoid reallocations
        //column.data.resize(column.data.size() - compSize);
    }

    return {lastRow, m_entities[row]};
}

} // namespace hex