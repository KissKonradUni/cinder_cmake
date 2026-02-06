#include <catch2/catch_test_macros.hpp>

#include "host.hpp"

using namespace cinder;

struct closeLayer: public Layer {
    closeLayer() = default;
    ~closeLayer() override = default;

    PhaseState onUpdate() override {
        if (m_host->getTime().totalTime > 0.1) {
            m_host->close();
        }
        return PhaseState::Continue;
    }
};

TEST_CASE("Engine can run in headless mode", "[engine][headless]") {
    REQUIRE(true);

    Host host;
    host.pushLayer<closeLayer>();
    host.run(0, nullptr);
}

TEST_CASE("Entity IDs are reused safely", "[engine][ecs]") {
    Time time;
    ComponentRegistry cr;
    World w(cr, &time);

    Entity a = w.createEntity();
    w.destroyEntity(a);

    Entity b = w.createEntity();

    REQUIRE(a.id == b.id);
    REQUIRE(a.generation != b.generation);
}

TEST_CASE("Invalid entities are detected", "[engine][ecs]") {
    Time time;
    ComponentRegistry cr;
    World w(cr, &time);

    Entity a = w.createEntity();
    w.destroyEntity(a);

    REQUIRE(!w.isEntityValid(a));

    Entity b = w.createEntity();
    REQUIRE(w.isEntityValid(b));
    REQUIRE(a.id == b.id);
    REQUIRE(a.generation != b.generation);
}

struct ExampleComponent {
    hex::vec4 data;
};

TEST_CASE("Add/remove component", "[engine][ecs]") {
    ComponentRegistry cr;
    auto compID = cr.registerComponent(ComponentDescriptor{
        .stableName = "ExampleComponent",
        .size = sizeof(ExampleComponent),
        .alignment = alignof(ExampleComponent)
    });

    Time time;
    World w(cr, &time);
    Entity e = w.createEntity();

    // Add component
    w.addComponents(e, { compID.value() });
    const auto compPtr = w.getComponent<ExampleComponent>(e, compID.value());
    REQUIRE(compPtr.has_value());
    ExampleComponent* comp = compPtr.value();
    comp->data.x = 1.0f;
    comp->data.y = 2.0f;
    comp->data.z = 3.0f;
    comp->data.w = 4.0f; 
    
    // Reaquire the component and check values
    const auto compPtr2 = w.getComponent<ExampleComponent>(e, compID.value());
    REQUIRE(compPtr2.has_value());
    ExampleComponent* comp2 = compPtr2.value();
    REQUIRE(comp2->data.x == 1.0f);
    REQUIRE(comp2->data.y == 2.0f);
    REQUIRE(comp2->data.z == 3.0f);
    REQUIRE(comp2->data.w == 4.0f); 

    // Remove component (should print a warning but not crash)
    w.removeComponents(e, { compID.value() });
    const auto compPtr3 = w.getComponent<ExampleComponent>(e, compID.value());
    REQUIRE(!compPtr3.has_value());
}
