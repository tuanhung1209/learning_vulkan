#pragma once

#include "game/my_game_object.hpp"
#include "game/terrain_generation.hpp"
#include "render_systems/sky_render_system.hpp"
#include "render_systems/grass_render_system.hpp"
#include "render_systems/ocean_render_system.hpp"

#include <glm/glm.hpp>

// might change this later
namespace my {

struct SceneEntityRef {
    MyGameObject::Map &gameObjects;
    MyGameObject::id_t playerId;
    TerrainGenerator::TerrainConfig &terrainConfig;
    SkyRenderSystem::SkyPush &skyConfig;
    GrassRenderSystem::GrassComputePush &grassConfig;
    OceanRenderSystem::OceanUbo &oceanConfig;

    glm::vec4 sunDirection{1.f, 0.5f, 0.f, 0.5f};
};

} // namespace my
