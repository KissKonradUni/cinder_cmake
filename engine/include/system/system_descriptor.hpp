#pragma once

#include "world/bitset.hpp"
#include "host_phase.hpp"

namespace cinder {
    class Host;
}

namespace hex {

using SystemID = uint32_t;

enum class SystemPhase: uint8_t {
    PreUpdate    = (uint8_t)cinder::HostPhase::PreUpdate,
    Update       = (uint8_t)cinder::HostPhase::Update,
    PostUpdate   = (uint8_t)cinder::HostPhase::PostUpdate,
    PrepareFrame = (uint8_t)cinder::HostPhase::PrepareFrame,
    RenderFrame  = (uint8_t)cinder::HostPhase::RenderFrame,
    PostFrame    = (uint8_t)cinder::HostPhase::PostFrame
};

struct SystemDescriptor {
friend class cinder::Host;
public:
    SystemDescriptor(SystemPhase phase = SystemPhase::Update,
                     const DynBitset& reads = DynBitset(),
                     const DynBitset& writes = DynBitset(),
                     bool multithreaded = false) : phase(phase), reads(reads), writes(writes), multithreaded(multithreaded), m_id(0) {}

    SystemPhase phase;
    DynBitset reads;
    DynBitset writes;
    bool multithreaded;

    inline SystemID getID() const { return m_id; }
protected:
    SystemID m_id;
};

};