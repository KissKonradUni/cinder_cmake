#pragma once

#include <cstdint>

namespace cinder {
    enum class HostPhase: uint8_t {
        Attach,
        Detach,
        Other,
        Update,
        PostUpdate,
        PrepareFrame,
        RenderFrame,
        PostFrame
    };
};