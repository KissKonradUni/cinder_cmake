#include "host.hpp"

#include "layers/wasm.hpp"
#include "layers/imgui.hpp"
#include "layers/editor.hpp"
#include "layers/window.hpp"
#include "layers/devtools.hpp"
#include "layers/gpu_device.hpp"

static uint32_t s_nameComponentCounter = 0;
struct NameComponent {
    char name[64];
    
    NameComponent() : name() {
        strcpy(name, std::format("Unnamed_{:02}", s_nameComponentCounter++).c_str());
    }
    ~NameComponent() = default;
};

struct Vec3 {
    float x, y, z;
};

struct TransformStruct {
    Vec3 position;
    Vec3 rotation;
    Vec3 scale;
};

hex::ComponentTypeID componentID;
hex::ComponentTypeID transformComponentID;

void moveTransforms(hex::WorldView& view) {
    auto transforms = view.readWriteRef(transformComponentID);
    if (!transforms.has_value()) {
        return;
    }

    hex::ComponentPool* pool = transforms.value();
    auto components = pool->getAll<TransformStruct>();

    int index = 0;
    for (auto& comp : components) {
        index ++;
        comp.position.x += 0.01f * index;
        comp.position.y += 0.01f * index;
        comp.position.z += 0.01f * index;
    }
}

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
    auto wasmFilePath = currentPath / "build/wasm_modules/wasm_example_module.wasm";
    auto wasmLayer = host.pushLayer<WasmLayer>(wasmFilePath);

    auto& componentRegistry = host.getComponentRegistry();
    componentID = componentRegistry
        .registerComponent(quick_component_desc<NameComponent>(
            "EntityNameComponent"
        )).value();
    transformComponentID = componentRegistry
        .registerComponent(quick_component_desc<TransformStruct>(
            "TransformComponent"
        )).value();

    auto& world = host.getWorld();

    for (int i = 0; i < 10; i++) {
        auto entity = world.createEntity();
        world.addComponents(entity, {componentID, transformComponentID});
    }

    DynBitset bitset;
    bitset.set(transformComponentID);

    SystemDescriptor moveTransformDesc(
        SystemPhase::Update,
        bitset,
        bitset,
        true
    );

    host.registerSystem(moveTransformDesc, moveTransforms);

    // Run host
    auto result = host.run(argc, argv);
    return result;
}