#include "host.hpp"

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

    // Print running directory
    auto currentPath = std::filesystem::current_path();
    std::println("Running directory: {}", currentPath.string());
 
    auto wasmLayer = host.pushLayer<WasmLayer>(currentPath / "build/wasm_modules/wasm_example_module.wasm");

    auto devtoolsLayer = host.pushLayer<DevToolsLayer>();

    auto result = host.run(argc, argv);
    return result;
}