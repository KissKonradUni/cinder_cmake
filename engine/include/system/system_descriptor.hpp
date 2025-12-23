#pragma once

#include "world/bitset.hpp"
#include "host_phase.hpp"

namespace hex {

struct SystemDescriptor {
    cinder::HostPhase phase;
    DynBitset reads;
    DynBitset writes;
    bool multithreaded;
};

};