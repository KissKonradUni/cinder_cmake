#include "host.hpp"
#include "layers/window.hpp"
#include "layers/gpu_device.hpp"
#include "layers/imgui.hpp"

int main(int argc, char* argv[]) {
    using namespace cinder;
    using namespace prism;
    using namespace echo;

    Host host;
    auto windowLayer = host.pushLayer<WindowLayer>();
    auto gpuDeviceLayer = host.pushLayer<GPUDeviceLayer>(&windowLayer);
    auto imguiLayer = host.pushLayer<ImGuiLayer>(&windowLayer, &gpuDeviceLayer);

    auto result = host.run(argc, argv);
    return result;
}