#pragma once

#include "ecs/ecs_manager.hpp"
#include "ecs/entity.hpp"

#include "render_systems/sky_render_system.hpp"
#include "render_systems/grass_render_system.hpp"
#include "render_systems/ocean_render_system.hpp"

#include "game/game_components/terrain_handler.hpp"

#include <glm/glm.hpp>

namespace my {

struct SceneEntityRef {
    EcsManager &ecsManager;
    Entity playerEntity;
    TerrainHandler::TerrainConfig &terrainConfig;
    SkyRenderSystem::SkyPush &skyConfig;
    GrassRenderSystem::GrassComputePush &grassConfig;
    OceanRenderSystem::OceanUbo &oceanConfig;

    glm::vec4 sunDirection{1.f, 0.5f, 0.f, 0.5f};
};

} // namespace my