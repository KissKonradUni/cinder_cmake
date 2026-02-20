#include "world/bitset.hpp"

namespace hex {

DynBitset::DynBitset() {}

DynBitset::DynBitset(uint32_t bitCount) {
    this->m_dataLength = (bitCount + 63) / 64;    
    this->m_data = new uint64_t[this->m_dataLength]{0};
}

DynBitset::DynBitset(const std::span<const DynBitset>& flags) {
    if (flags.empty()) {
        this->m_data = new uint64_t[1]{0};
        return;
    }

    auto maxSize = flags[0].m_dataLength;
    for (const auto& flag : flags) {
        if (flag.m_dataLength > maxSize) {
            maxSize = flag.m_dataLength;
        }
    }

    this->m_dataLength = maxSize;
    this->m_data = new uint64_t[this->m_dataLength]{0};
    for (const auto& flag : flags) {
        for (size_t i = 0; i < flag.m_dataLength; ++i) {
            this->m_data[i] |= flag.m_data[i];
        }
    }
}

void DynBitset::set(uint32_t bit) {
    const uint32_t index = bit / 64;
    const uint32_t offset = bit % 64;
    if (index >= m_dataLength) {
        // Resize data array
        uint32_t newLength = index + 1;
        uint64_t* newData = new uint64_t[newLength]{0};
        for (uint32_t i = 0; i < m_dataLength; ++i) {
            newData[i] = m_data[i];
        }
        delete[] m_data;
        m_data = newData;
        m_dataLength = newLength;
    }
    m_data[index] |= (uint64_t(1) << offset);
}

void DynBitset::remove(uint32_t bit) {
    const uint32_t index = bit / 64;
    const uint32_t offset = bit % 64;
    if (index >= m_dataLength) {
        return;
    }
    m_data[index] &= ~(uint64_t(1) << offset);
}

void DynBitset::clear() {
    for (uint32_t i = 0; i < m_dataLength; ++i) {
        m_data[i] = 0;
    }
}

bool DynBitset::test(uint32_t bit) const {
    const uint32_t index = bit / 64;
    const uint32_t offset = bit % 64;
    if (index >= m_dataLength) {
        return false;
    }
    return (m_data[index] & (uint64_t(1) << offset)) != 0;
}

bool DynBitset::contains(const DynBitset& other) const {
    const size_t minSize = std::min(m_dataLength, other.m_dataLength);
    for (size_t i = 0; i < minSize; ++i) {
        if ((m_data[i] & other.m_data[i]) != other.m_data[i]) {
            return false;
        }
    }
    for (size_t i = minSize; i < other.m_dataLength; ++i) {
        if (other.m_data[i] != 0) {
            return false;
        }
    }
    return true;
}

uint32_t DynBitset::bitCount() const {
    uint32_t count = 0;
    for (uint32_t i = 0; i < m_dataLength; ++i) {
        uint64_t block = m_data[i];
        count += __builtin_popcountll(block);
    }
    return count;
}

uint32_t DynBitset::bitLength() const {
    return m_dataLength * 64;
}

DynBitset DynBitset::operator|(const DynBitset& other) const {
    DynBitset result;
    const size_t maxSize = std::max(m_dataLength, other.m_dataLength);
    result.m_dataLength = maxSize;
    result.m_data = new uint64_t[maxSize]{0};
    for (size_t i = 0; i < maxSize; ++i) {
        uint64_t thisBlock = (i < m_dataLength) ? m_data[i] : 0;
        uint64_t otherBlock = (i < other.m_dataLength) ? other.m_data[i] : 0;
        result.m_data[i] = thisBlock | otherBlock;
    }
    return result;
}

DynBitset DynBitset::operator&(const DynBitset& other) const {
    DynBitset result;
    const size_t minSize = std::min(m_dataLength, other.m_dataLength);
    result.m_dataLength = minSize;
    result.m_data = new uint64_t[minSize]{0};
    for (size_t i = 0; i < minSize; ++i) {
        result.m_data[i] = m_data[i] & other.m_data[i];
    }
    return result;
}

bool DynBitset::operator==(const DynBitset& other) const {
    if (m_dataLength != other.m_dataLength) {
        return false;
    }
    for (size_t i = 0; i < m_dataLength; ++i) {
        if (m_data[i] != other.m_data[i]) {
            return false;
        }
    }
    return true;
}

} // namespace hex