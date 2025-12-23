#include "host.hpp"

#include "layers/wasm.hpp"
#include "layers/imgui.hpp"
#include "layers/editor.hpp"
#include "layers/window.hpp"
#include "layers/devtools.hpp"
#include "layers/gpu_device.hpp"

struct NameComponent {
    char name[64];
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
    std::println("Running directory: {}", currentPath.string());
    auto wasmFilePath = currentPath / "build/wasm_modules/wasm_example_module.wasm";
    auto wasmLayer = host.pushLayer<WasmLayer>(wasmFilePath);

    auto& componentRegistry = host.getComponentRegistry();
    componentID = componentRegistry.registerComponent(ComponentDescriptor{
        .stableName = "EntityNameComponent",
        .size = sizeof(NameComponent),
        .alignment = alignof(NameComponent)
    }).value();
    transformComponentID = componentRegistry.registerComponent(ComponentDescriptor{
        .stableName = "TransformComponent",
        .size = sizeof(TransformStruct),
        .alignment = alignof(TransformStruct)
    }).value();

    auto& world = host.getWorld();

    for (int i = 0; i < 10; i++) {
        auto entity = world.createEntity();
        world.addComponents(entity, {componentID, transformComponentID});
        auto nameComponentOpt = world.getComponent<NameComponent>(entity, componentID);
        if (nameComponentOpt.has_value()) {
            std::string name = "Entity_" + std::to_string(entity.id);
            strncpy((*nameComponentOpt.value()).name, name.c_str(), sizeof((*nameComponentOpt.value()).name));

            std::println("Created entity ID {} with name {}", entity.id, nameComponentOpt.value()->name);
        }
    }

    DynBitset readBits;
    readBits.set(transformComponentID);
    DynBitset writeBits;
    writeBits.set(transformComponentID);

    SystemDescriptor moveTransformDesc(
        SystemPhase::Update,
        readBits,
        writeBits,
        true
    );

    host.registerSystem(moveTransformDesc, moveTransforms);

    // Run host
    auto result = host.run(argc, argv);
    return result;
}