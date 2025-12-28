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
    vec3 position;
    vec3 rotation;
    vec3 scale;
};

struct ExampleGarbageComponent {
    float anotherValue = 3.14f;
    int someValue = 42;
    vec2 anotherVector = {0.5f, 0.75f};
    vec3 vectorValue = {1.0f, 2.0f, 3.0f};
    vec4 colorValue = {1.0f, 0.0f, 0.0f, 1.0f};
    mat3 someMatrix = {
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    };
    mat4 anotherMatrix = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    cinder::str someString;

    ExampleGarbageComponent() {
        cinder::str_init(someString, 64);
        cinder::str_copy(someString, "Hello, here is some garbage string!");
    }

    ~ExampleGarbageComponent() {
        cinder::str_free(someString);
    }
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
        comp.position.y = index * 2;
        comp.position.z = index * 3;
        index++;
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

    //auto currentPath = std::filesystem::current_path();
    //auto wasmFilePath = currentPath / "build/wasm_modules/wasm_example_module.wasm";
    //auto wasmLayer = host.pushLayer<WasmLayer>(wasmFilePath);

    auto& componentRegistry = host.getComponentRegistry();
    auto componentID = componentRegistry
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
    auto garbageComponentID = componentRegistry
        .registerComponent(quick_component_desc<ExampleGarbageComponent>(
            "ExampleGarbageComponent", {
                {.name = "anotherValue", .type = as_field_enum<float>(),       .offset = offsetof(ExampleGarbageComponent, anotherValue)},
                {.name = "someValue",    .type = as_field_enum<int>(),         .offset = offsetof(ExampleGarbageComponent, someValue)},
                {.name = "anotherVector",.type = as_field_enum<vec2>(),        .offset = offsetof(ExampleGarbageComponent, anotherVector)},
                {.name = "vectorValue",  .type = as_field_enum<vec3>(),        .offset = offsetof(ExampleGarbageComponent, vectorValue)},
                {.name = "colorValue",   .type = as_field_enum<vec4>(),        .offset = offsetof(ExampleGarbageComponent, colorValue)},
                {.name = "someMatrix",   .type = as_field_enum<mat3>(),        .offset = offsetof(ExampleGarbageComponent, someMatrix)},
                {.name = "anotherMatrix",.type = as_field_enum<mat4>(),        .offset = offsetof(ExampleGarbageComponent, anotherMatrix)},
                {.name = "someString",   .type = as_field_enum<cinder::str>(), .offset = offsetof(ExampleGarbageComponent, someString)},
            }
        )).value();

    auto& world = host.getWorld();

    for (int i = 0; i < 100; i++) {
        auto entity = world.createEntity();
        world.addComponents(entity, {componentID, transformComponentID, garbageComponentID});
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