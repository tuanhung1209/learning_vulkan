#include "game/first_app.hpp"
#include "game/keyboard_movement_controller.hpp"
#include "game/my_player.hpp"
#include "game/physics_utils.hpp"
#include "game/scene_file_panel.hpp"
#include "game/terrain_generation.hpp"
#include "imgui.h"
#include "input/glfw_input_bridge.hpp"
#include "input/input_state.hpp"
#include "math/NOAA_solar_position.hpp"
#include "my_save_system.hpp"
#include "render_core/my_frame_info.hpp"
#include "render_core/my_imgui.hpp"
#include "render_core/my_texture.hpp"
#include "render_systems/grass_render_system.hpp"
#include "render_systems/ocean_render_system.hpp"
#include "render_systems/point_light_system.hpp"
#include "render_systems/simple_render_system.hpp"
#include "render_systems/sky_render_system.hpp"
#include "vulkan_core/my_buffer.hpp"
#include "vulkan_core/swap_chain.hpp"

#include <GLFW/glfw3.h>
#include <chrono>
#include <cstdio>
#include <glm/common.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace my {

FirstApp::FirstApp(Mode mode) : mode_(mode) {
    if (mode_ == Mode::Edit) {
        glfwWindow = std::make_unique<GlfwWindow>(WIDTH, HEIGHT, "wallpaperEdit");

        device = std::make_unique<Device>([this](VkInstance inst) {
            glfwWindow->createVulkanSurface(inst);
            return glfwWindow->getVkSurface();
        });

        myRenderer = std::make_unique<MyRenderer>(*glfwWindow, *device);
    } else {
        waylandWindow = std::make_unique<WaylandWindow>("wallpaper");

        device = std::make_unique<Device>([this](VkInstance isnt) {
            waylandWindow->createVulkanSurfaces(isnt);
            return waylandWindow->getMonitor()[0].vkSurface;
        });

        myRenderer = std::make_unique<MyRenderer>(*waylandWindow, *device);
    }

    globalPool = MyDescriptorPool::Builder(*device)
                     .setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT)
                     .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SwapChain::MAX_FRAMES_IN_FLIGHT)
                     .build();
    loadGameObjects();
}

FirstApp::~FirstApp() {
    myRenderer.reset();

    if (mode_ == Mode::Edit) {
        glfwWindow->destroyVulkanSurfaces(device->getInstance());
    } else {
        waylandWindow->destroyVulkanSurfaces(device->getInstance());
    }
}

void FirstApp::run() {
    std::vector<std::unique_ptr<MyBuffer>> uboBuffers(SwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < uboBuffers.size(); i++) {
        uboBuffers[i] =
            std::make_unique<MyBuffer>(*device, sizeof(GlobalUbo), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        uboBuffers[i]->map();
    }

    auto globalSetLayout = MyDescriptorSetLayout::Builder(*device)
                               .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                           VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_COMPUTE_BIT)
                               .build();

    std::vector<VkDescriptorSet> globalDescriptorSet(SwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < globalDescriptorSet.size(); i++) {
        auto bufferInfo = uboBuffers[i]->descriptorInfo();
        MyDescriptorWriter(*globalSetLayout, *globalPool)
            .writeBuffer(0, &bufferInfo)
            .build(globalDescriptorSet[i]);
    }

    SimpleRenderSystem simpleRenderSystem{*device, myRenderer->getSceneRenderPass(),
                                          globalSetLayout->getDescriptorSetLayout()};
    PointLightSystem PointLightSystem{*device, myRenderer->getSceneRenderPass(),
                                      globalSetLayout->getDescriptorSetLayout()};
    SkyRenderSystem skyRenderSystem{*device, myRenderer->getSceneRenderPass(),
                                    globalSetLayout->getDescriptorSetLayout()};
    GrassRenderSystem grassRenderSystem{*device, myRenderer->getSceneRenderPass(),
                                        globalSetLayout->getDescriptorSetLayout()};
    OceanRenderSystem oceanRenderSystem{*device, myRenderer->getSceneRenderPass(),
                                        globalSetLayout->getDescriptorSetLayout()};
    TerrainGenerator terrainGen{*device};

    std::unique_ptr<ImGuiWrapper> guiRenderSystem;
    if (mode_ == Mode::Edit) {
        guiRenderSystem =
            std::make_unique<ImGuiWrapper>(*device, *glfwWindow, myRenderer->getSceneRenderPass());
    }

    SaveSystem saveSystem{*device};
    SceneFilePanel sceneFilePanel{};

    std::shared_ptr<MyModel> bulletModel = MyModel::createModelFromFile(*device, "assets/models/cube.obj");
    BulletHandler bulletHandler{bulletModel};

    InputState inputState{};
    std::unique_ptr<GlfwInput> glfwInput;
    if (mode_ == Mode::Edit) { glfwInput = std::make_unique<GlfwInput>(*glfwWindow, inputState); }

    MyCamera camera{};
    camera.setViewTarget(glm::vec3(-1.f, -2.f, 2.f), glm::vec3(0.f, 0.f, 2.5f));

    auto playerObject = MyGameObject::createGameObject();
    auto playerId = playerObject.getId();
    gameObjects.emplace(playerId, std::move(playerObject));
    MyPlayer mainPlayer{camera, playerId, inputState};

    PhysicsWorld physicsWorld;

    SceneEntityRef sceneRef{gameObjects,
                            playerId,
                            terrainGen.config,
                            skyRenderSystem.getPush(),
                            grassRenderSystem.getPush(),
                            oceanRenderSystem.getOceanUbo()};

    saveSystem.loadScene("assets/scenes/default.json", sceneRef);
    terrainGen.createTerrain(gameObjects);
    grassRenderSystem.updateHeightMap(terrainGen.getHeightMap(), terrainGen.config.heightScale);

    auto currentTime = std::chrono::high_resolution_clock::now();
    float totalTime = 0.f;

    // TEST :
    float timeOfDay = 10.5f;
    float latitude = 21.0278f;
    float longitude = 105.8342f;
    int year = 2026;
    int month = 6;
    int day = 23;

    while (mode_ == Mode::Wallpaper ? !waylandWindow->shouldClose() : !glfwWindow->shouldClose()) {
        if (mode_ == Mode::Wallpaper) {
            waylandWindow->pollEvents();
        } else {
            glfwPollEvents();
            glfwInput->pollKeyboardFromGlfw(*glfwWindow);
            glfwInput->pollMouseFromGlfw(*glfwWindow);
        }

        auto newTime = std::chrono::high_resolution_clock::now();
        auto frameTime =
            std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;
        totalTime += frameTime;

        mainPlayer.update(inputState, frameTime, gameObjects, bulletHandler);
        bulletHandler.update(frameTime);
        physicsWorld.step(gameObjects, bulletHandler.getBullets(), frameTime);

        float aspect = myRenderer->getAspectRatio();
        camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 10000.f);

        if (auto commandBuffer = myRenderer->beginFrame()) {
            int frameIndex = myRenderer->getFrameIndex();
            FrameInfo frameInfo{frameIndex, frameTime, commandBuffer, camera, globalDescriptorSet[frameIndex],
                                gameObjects};

            if (guiRenderSystem) {
                guiRenderSystem->newFrame();
                ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(),
                                             ImGuiDockNodeFlags_PassthruCentralNode);

                auto event = sceneFilePanel.drawGui(*glfwWindow);
                if (event.savePath) { saveSystem.saveScene(*event.savePath, sceneRef); }
                if (event.loadPath) {
                    vkDeviceWaitIdle(device->device());
                    gameObjects.clear();

                    auto newPlayer = MyGameObject::createGameObject();
                    sceneRef.playerId = newPlayer.getId();
                    gameObjects.emplace(sceneRef.playerId, std::move(newPlayer));
                    mainPlayer.setPlayerId(sceneRef.playerId);

                    saveSystem.loadScene(*event.loadPath, sceneRef);
                    terrainGen.createTerrain(gameObjects);
                    grassRenderSystem.updateHeightMap(terrainGen.getHeightMap(),
                                                      terrainGen.config.heightScale);
                }

                if (terrainGen.drawGui()) {
                    vkDeviceWaitIdle(device->device());
                    terrainGen.regenerate(gameObjects);
                    grassRenderSystem.updateHeightMap(terrainGen.getHeightMap(),
                                                      terrainGen.config.heightScale);
                }

                grassRenderSystem.drawGui(terrainGen.config.heightScale);
                skyRenderSystem.drawGui();
                oceanRenderSystem.drawGui();

                ImGui::Begin("Sun");
                ImGui::SliderFloat("Time of Day (hours)", &timeOfDay, 0.0f, 24.0f);
                ImGui::Separator();
                ImGui::InputFloat("Latitude", &latitude);
                ImGui::InputFloat("Longitude", &longitude);
                ImGui::InputInt("Year", &year);
                ImGui::InputInt("Month", &month);
                ImGui::InputInt("Day", &day);

                {
                    float utcOffset = longitude / 15.0f;
                    float utc = timeOfDay - utcOffset;
                    int uh = static_cast<int>(utc);
                    int um = static_cast<int>((utc - uh) * 60.0f);
                    float us = ((utc - uh) * 60.0f - um) * 60.0f;
                    SolarResult sr =
                        SolarPosition::calculate(latitude, longitude, year, month, day, uh, um, us);
                    float elev = glm::pi<float>() / 2.0f - sr.zenith;
                    ImGui::Text("Elevation: %.1f deg", glm::degrees(elev));
                }
                ImGui::End();
            }

            // line below to update GlobalUbo
            GlobalUbo ubo{};

            ubo.projection = camera.getProjectionMatrix();
            ubo.view = camera.getView();
            ubo.inverseView = camera.getInverseView();

            ubo.fogColor = skyRenderSystem.getPush().horizonColor;
            ubo.fogNear = skyRenderSystem.getFog().near;
            ubo.fogFar = skyRenderSystem.getFog().far;
            ubo.fogDensity = skyRenderSystem.getFog().density;
            ubo.time = totalTime;

            float utcOffset = longitude / 15.0f;
            float utc = timeOfDay - utcOffset;
            int utcHour = static_cast<int>(utc);
            int utcMinute = static_cast<int>((utc - utcHour) * 60.0f);
            float utcSecond = ((utc - utcHour) * 60.0f - utcMinute) * 60.0f;

            SolarResult solarResult = SolarPosition::calculate(latitude, longitude, year, month, day, utcHour,
                                                               utcMinute, utcSecond);
            float elevation = glm::pi<float>() / 2.0f - solarResult.zenith;
            glm::vec3 sunDirection = SolarPosition::toDirection(solarResult);

            ubo.sunDirection = glm::vec4(-sunDirection, elevation);

            PointLightSystem.update(frameInfo, ubo);

            uboBuffers[frameIndex]->writeToBuffer(&ubo);
            uboBuffers[frameIndex]->flush();

            // line below for compute
            grassRenderSystem.computeGrass(frameInfo);

            // line below to render
            myRenderer->beginSwapChainRenderPass(commandBuffer);

            skyRenderSystem.renderSky(frameInfo);
            oceanRenderSystem.renderOcean(frameInfo);
            simpleRenderSystem.renderGameObjects(frameInfo);
            bulletHandler.renderBullet(commandBuffer, simpleRenderSystem.getGraphicPipelineLayout());
            grassRenderSystem.renderGrass(frameInfo);
            PointLightSystem.renderLight(frameInfo);

            if (guiRenderSystem) { guiRenderSystem->renderGui(frameInfo); }

            myRenderer->endSwapChainRenderPass(commandBuffer);
            myRenderer->endFrame();
        }
        inputState.endFrame();
    }
    vkDeviceWaitIdle(device->device());
}

void FirstApp::loadGameObjects() {
    auto grassTexture = std::make_shared<MyTexture>(*device, "assets/textures/grass.png");
    auto waterTexture = std::make_shared<MyTexture>(*device, "assets/textures/water.jpg");

    std::shared_ptr<MyModel> cubeModel =
        MyModel::createModelFromFile(*device, "assets/models/colored_cube.obj");
    std::shared_ptr<MyModel> quadModel = MyModel::createModelFromFile(*device, "assets/models/quad.obj");
    std::shared_ptr<MyModel> smoothVase =
        MyModel::createModelFromFile(*device, "assets/models/smooth_vase.obj");
    std::shared_ptr<MyModel> roughVase = MyModel::createModelFromFile(*device, "assets/models/flat_vase.obj");
    std::shared_ptr<MyModel> sphereModel = MyModel::createModelFromFile(*device, "assets/models/sphere.obj");
}

} // namespace my
