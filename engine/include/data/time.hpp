#pragma once

namespace cinder {

struct Time {
    float renderDelta = 0.0f;
    float updateDelta = 0.0f;
    
    double lastUpdateTime = 0.0f;
    double lastRenderTime = 0.0f;
    double totalTime = 0.0f;
};

} // namespace cinder