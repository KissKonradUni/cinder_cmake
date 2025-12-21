#include "host.hpp"

#include "world/world.hpp"
#include "world/entity.hpp"

#include "layers/wasm.hpp"
#include "layers/imgui.hpp"
#include "layers/window.hpp"
#include "layers/devtools.hpp"
#include "layers/gpu_device.hpp"

#include <print>

int main(int argc, char* argv[]) {
    using namespace cinder;
    using namespace prism;
    using namespace echo;
    using namespace hex;

    Host host;
    auto windowLayer = host.pushLayer<WindowLayer>();
    auto gpuDeviceLayer = host.pushLayer<GPUDeviceLayer>(windowLayer);
    auto imguiLayer = host.pushLayer<ImGuiLayer>(windowLayer, gpuDeviceLayer, "assets/fonts/Electrolize-Regular.ttf");
    auto devtoolsLayer = host.pushLayer<DevToolsLayer>();

    // Print running directory
    // auto currentPath = std::filesystem::current_path();
    // std::println("Running directory: {}", currentPath.string());
    // auto wasmLayer = host.pushLayer<WasmLayer>(currentPath / "build/wasm_modules/wasm_example_module.wasm");

    auto componentRegistry = host.getComponentRegistry();
    auto compID = componentRegistry->registerComponent({
        .stableName = "Transform",
        .size = sizeof(float) * 16,
        .alignment = alignof(float)
    });

    auto compID2 = componentRegistry->registerComponent({
        .stableName = "Velocity",
        .size = sizeof(float) * 3,
        .alignment = alignof(float)
    });

    auto world = host.getWorld();
    for (int i = 0; i < 1000; i++) {
        Entity entity = world->createEntity();
        std::println("Created entity with ID: {}, generation: {}", entity.id, entity.generation);

        world->addComponents(entity, { compID });
        std::println("Added Transform component ({}) to entity ID: {}", compID, entity.id);
    }

    /*
    for (int i = 200; i < 300; i++) {
        const auto entity = Entity {
            .id = static_cast<uint32_t>(i),
            .generation = 0
        };
        world->destroyEntity(entity);
        std::println("Destroyed entity with ID: {}", entity.id);
    }
    */

    for (int i = 400; i < 600; i++) {
        const auto entity = Entity {
            .id = static_cast<uint32_t>(i),
            .generation = 0
        };
        world->addComponents(entity, { compID2 });
    }

    /*
    for (int i = 500; i < 700; i++) {
        const auto entity = Entity {
            .id = static_cast<uint32_t>(i),
            .generation = 0
        };
        world->destroyEntity(entity);
        std::println("Destroyed entity with ID: {}", entity.id);
    }
    */

    // Run host
    auto result = host.run(argc, argv);
    return result;
}