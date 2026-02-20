#include "host.hpp"

#include "data/str.hpp"
#include "data/math.hpp"

#include "layers/wasm.hpp"
#include "layers/imgui.hpp"
#include "layers/editor.hpp"
#include "layers/window.hpp"
#include "layers/devtools.hpp"
#include "layers/gpu_device.hpp"

static uint32_t s_nameComponentCounter = 0;
struct NameComponent {
    cinder::str name;
    
    NameComponent() : name() {
        cinder::str_init(name, 32);
        std::string defaultName = std::format("Entity_{:03}", s_nameComponentCounter++);
        cinder::str_copy(name, defaultName.c_str());
    }

    ~NameComponent() {
        cinder::str_free(name);
    }
};

struct TransformStruct {
    hex::vec3 position;
    hex::vec3 rotation;
    hex::vec3 scale;
};

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
        comp.position.x = index;
        comp.position.y = 0;
        comp.position.z = view.getTime()->totalTime;
        index++;
    }

    static double lastTime = 0.0;
    if (view.getTime()->totalTime - lastTime > 0.1) {
        lastTime = view.getTime()->totalTime;
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
    auto nameComponentID = componentRegistry
        .registerComponent(quick_component_desc<NameComponent>(
            "EntityNameComponent", {
                {.name = "name", .type = as_field_enum<cinder::str>(), .offset = offsetof(NameComponent, name)}
            }
        )).value();
    transformComponentID = componentRegistry
        .registerComponent(quick_component_desc<TransformStruct>(
            "TransformComponent", {
                {.name = "position", .type = as_field_enum<vec3>(), .offset = offsetof(TransformStruct, position)},
                {.name = "rotation", .type = as_field_enum<vec3>(), .offset = offsetof(TransformStruct, rotation)},
                {.name = "scale",    .type = as_field_enum<vec3>(), .offset = offsetof(TransformStruct, scale)}
            }
        )).value();

    auto& world = host.getWorld();

    for (int i = 0; i < 100; i++) {
        auto entity = world.createEntity();
        world.addComponents(entity, {nameComponentID, transformComponentID});
    }

    for (int i = 50; i < 75; i++) {
        auto entity = Entity(i, 0);
        world.destroyEntity(entity);
    }

    for (int i = 50; i < 75; i++) {
        auto entity = world.createEntity();
        world.addComponents(entity, {nameComponentID, transformComponentID});
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

    echo::logInfo("Starting host...");

    // Run host
    auto result = host.run(argc, argv);
    
    echo::logInfo(std::format("Host exited with code \"{}\".", result));

    return result;
}