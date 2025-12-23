#pragma once

#include <cstdint>

namespace cinder {
    enum class HostPhase: uint8_t {
        Attach,
        Detach,
        Other,

        PreUpdate,  // TODO: Used for ECS modification queue after last frame
        Update,
        PostUpdate,
        
        PrepareFrame,
        RenderFrame,
        PostFrame
    };
};