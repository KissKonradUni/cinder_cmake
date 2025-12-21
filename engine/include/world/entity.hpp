#pragma once

#include <cstdint>

namespace hex {

struct Entity {
    uint32_t id;
    uint32_t generation;
};

class Archetype;

struct EntityRecord {
    Archetype* archetype;
    uint32_t row; // index inside archetype storage
    uint32_t generation;
};

} // namespace hex