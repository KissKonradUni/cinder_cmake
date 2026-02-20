#pragma once

#include <cstdint>
#include <span>

namespace hex {

struct DynBitset {
    DynBitset();
    explicit DynBitset(uint32_t bitCount);
    explicit DynBitset(const std::span<const DynBitset>& flags);

    void set(uint32_t bit);
    void remove(uint32_t bit);
    void clear();
    bool test(uint32_t bit) const;

    bool contains(const DynBitset& other) const;

    /** The amount of bits set to one */
    uint32_t bitCount() const;
    /** The total length in bits */
    uint32_t bitLength() const;

    DynBitset operator|(const DynBitset& other) const;
    DynBitset operator&(const DynBitset& other) const;

    bool operator==(const DynBitset& other) const;
private:
    uint64_t* m_data;
    uint32_t m_dataLength;
};

} // namespace hex