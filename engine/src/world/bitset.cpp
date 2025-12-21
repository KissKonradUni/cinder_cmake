#include "world/bitset.hpp"

namespace hex {

DynBitset::DynBitset(): data(0) {}

DynBitset::DynBitset(uint32_t bitCount) {
    const uint32_t numU64 = (bitCount + 63) / 64;
    data.resize(numU64, 0);
}

DynBitset::DynBitset(const std::vector<DynBitset>& flags) {
    if (flags.empty()) {
        data.clear();
        return;
    }

    auto maxSize = flags[0].data.size();
    for (const auto& flag : flags) {
        if (flag.data.size() > maxSize) {
            maxSize = flag.data.size();
        }
    }

    data.resize(maxSize, 0);
    for (const auto& flag : flags) {
        for (size_t i = 0; i < flag.data.size(); ++i) {
            data[i] |= flag.data[i];
        }
    }
}

void DynBitset::set(uint32_t bit) {
    const uint32_t index = bit / 64;
    const uint32_t offset = bit % 64;
    if (index >= data.size()) {
        data.resize(index + 1, 0);
    }
    data[index] |= (uint64_t(1) << offset);
}

void DynBitset::remove(uint32_t bit) {
    const uint32_t index = bit / 64;
    const uint32_t offset = bit % 64;
    if (index >= data.size()) {
        return;
    }
    data[index] &= ~(uint64_t(1) << offset);
}

void DynBitset::clear() {
    for (auto& block : data) {
        block = 0;
    }
}

bool DynBitset::test(uint32_t bit) const {
    const uint32_t index = bit / 64;
    const uint32_t offset = bit % 64;
    if (index >= data.size()) {
        return false;
    }
    return (data[index] & (uint64_t(1) << offset)) != 0;
}

bool DynBitset::contains(const DynBitset& other) const {
    const size_t minSize = std::min(data.size(), other.data.size());
    for (size_t i = 0; i < minSize; ++i) {
        if ((data[i] & other.data[i]) != other.data[i]) {
            return false;
        }
    }
    for (size_t i = minSize; i < other.data.size(); ++i) {
        if (other.data[i] != 0) {
            return false;
        }
    }
    return true;
}

uint32_t DynBitset::bitCount() const {
    uint32_t count = 0;
    for (const auto& block : data) {
        count += __builtin_popcountll(block);
    }
    return count;
}

uint32_t DynBitset::bitLength() const {
    return data.size() * 64;
}

DynBitset DynBitset::operator|(const DynBitset& other) const {
    DynBitset result;
    const size_t maxSize = std::max(data.size(), other.data.size());
    result.data.resize(maxSize, 0);
    for (size_t i = 0; i < maxSize; ++i) {
        uint64_t a = (i < data.size()) ? data[i] : 0;
        uint64_t b = (i < other.data.size()) ? other.data[i] : 0;
        result.data[i] = a | b;
    }
    return result;
}

DynBitset DynBitset::operator&(const DynBitset& other) const {
    DynBitset result;
    const size_t minSize = std::min(data.size(), other.data.size());
    result.data.resize(minSize, 0);
    for (size_t i = 0; i < minSize; ++i) {
        result.data[i] = data[i] & other.data[i];
    }
    return result;
}

bool DynBitset::operator==(const DynBitset& other) const {
    return data == other.data;
}

} // namespace hex