#include "ecs/components/point_light_component.hpp"
#include "ecs/components/transform_component.hpp"
#include "ecs/ecs_manager.hpp"

#include <cstdio>

namespace my {

// Throwaway scratch test that exercises Entity/ComponentPool/EcsManager/Query
// before we migrate any real system onto the registry. Lives here so it never
// includes my_game_object.hpp (which would collide with TransformComponent).
//
// Run with:   ./build/VulkanOut --test-ecs

void runEcsTest() {
    std::printf("=== ECS scratch test ===\n");

    EcsManager registry;

    // 3 entities
    Entity e0 = registry.createEntity();
    Entity e1 = registry.createEntity();
    Entity e2 = registry.createEntity();

    // everyone gets a Transform
    registry.add<TransformComponent>(e0, TransformComponent{});
    registry.add<TransformComponent>(e1, TransformComponent{});
    registry.add<TransformComponent>(e2, TransformComponent{});

    // e0 & e1 are lights (have PointLight + Color); e2 is NOT a light
    registry.add<PointLightComponent>(e0, PointLightComponent{10.f});
    registry.add<PointLightComponent>(e1, PointLightComponent{5.f});
    registry.add<ColorComponent>(e0, ColorComponent{{1.f, 0.f, 0.f}});
    registry.add<ColorComponent>(e1, ColorComponent{{0.f, 1.f, 0.f}});

    // --- 1) single-component query: all 3 have Transform, expect 3
    int tCount = 0;
    for (auto [e, t] : registry.query<TransformComponent>()) {
        (void)e;
        (void)t;
        tCount++;
    }
    std::printf("query<Transform>                  -> %d (expect 3)\n", tCount);

    // --- 2) multi-component query: only e0 & e1 have all 3, expect 2
    int lightCount = 0;
    for (auto [e, t, pl, c] : registry.query<TransformComponent, PointLightComponent, ColorComponent>()) {
        std::printf("  light: id=%u gen=%u intensity=%.1f color=(%.1f,%.1f,%.1f) pos=(%.1f,%.1f,%.1f)\n",
                    e.id, e.generation, pl.lightIntensity, c.rgb.x, c.rgb.y, c.rgb.z, t.translation.x,
                    t.translation.y, t.translation.z);
        lightCount++;
    }
    std::printf("query<Transform,Light,Color>     -> %d (expect 2)\n", lightCount);

    // --- 3) destroy e0 -> should disappear from the light query, expect 1
    registry.destroyEntity(e0);
    int afterDestroy = 0;
    for (auto [e, t, pl, c] : registry.query<TransformComponent, PointLightComponent, ColorComponent>()) {
        (void)e;
        (void)t;
        (void)pl;
        (void)c;
        afterDestroy++;
    }
    std::printf("after destroy e0                  -> %d (expect 1)\n", afterDestroy);
    std::printf("isAlive(e0)?                      -> %d (expect 0)\n", registry.isEntityAlive(e0) ? 1 : 0);

    // --- 4) generation recycling: create new entity, should reuse e0's slot
    //         with a bumped generation so it != e0
    Entity e3 = registry.createEntity();
    std::printf("recycled e3 = {id=%u, gen=%u} (expect id=%u gen=1)\n", e3.id, e3.generation, e0.id);
    std::printf("isAlive(e3)?                      -> %d (expect 1)\n", registry.isEntityAlive(e3) ? 1 : 0);
    std::printf("e0 == e3?                         -> %d (expect 0)\n", e0 == e3 ? 1 : 0);

    std::printf("=== done ===\n");
}

} // namespace my
