#pragma once

#include <cstdint>

#include "world/bitset.hpp"
#include "world/component.hpp"

namespace hex {

struct Entity {
    uint32_t id;
    uint32_t generation;
};

struct ComponentRecord {
    ComponentTypeID type;
    uint32_t row;
};

struct EntityRecord {
    DynBitset componentBits;
    uint32_t generation;
    std::vector<ComponentRecord> components;
};

} // namespace hex