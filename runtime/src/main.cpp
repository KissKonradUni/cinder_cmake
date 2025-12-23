#include "host.hpp"

#include "layers/wasm.hpp"
#include "layers/imgui.hpp"
#include "layers/editor.hpp"
#include "layers/window.hpp"
#include "layers/devtools.hpp"
#include "layers/gpu_device.hpp"

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
    auto editorLayer = host.pushLayer<EditorLayer>(); 

    auto currentPath = std::filesystem::current_path();
    std::println("Running directory: {}", currentPath.string());
    auto wasmFilePath = currentPath / "build/wasm_modules/wasm_example_module.wasm";
    auto wasmLayer = host.pushLayer<WasmLayer>(wasmFilePath);

    auto& componentRegistry = host.getComponentRegistry();
    auto componentID = componentRegistry.registerComponent(ComponentDescriptor{
        .stableName = "EntityNameComponent",
        .size = sizeof(std::string),
        .alignment = alignof(std::string)
    });
    auto& world = host.getWorld();

    for (int i = 0; i < 10000; i++) {
        auto entity = world.createEntity();
        world.addComponents(entity, {componentID.value()});
        auto nameComponentOpt = world.getComponent<std::string>(entity, componentID.value());
        if (nameComponentOpt.has_value()) {
            std::string name = std::format("Test Entity {}", entity.id);
            *nameComponentOpt.value() = name;
        }
    }

    // Run host
    auto result = host.run(argc, argv);
    return result;
}