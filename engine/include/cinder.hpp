#pragma once

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

namespace cinder {

class Application {
public:
    Application() = default;
    ~Application() = default;

    int run(int argc, char* argv[]);
    void printDebugInfo();

    SDL_Window* window = nullptr;
    SDL_GPUDevice* device = nullptr;
};

} // namespace cinder