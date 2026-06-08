#include "game/first_app.hpp"
#include "game/keyboard_movement_controller.hpp"
#include "game/my_player.hpp"
#include "game/physics_utils.hpp"
#include "game/scene_file_panel.hpp"
#include "game/terrain_generation.hpp"
#include "imgui.h"
#include "input/glfw_input_bridge.hpp"
#include "input/input_state.hpp"
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

    SceneEntityRef sceneRef{gameObjects, playerId, terrainGen.config, skyRenderSystem.getPush(),
                            grassRenderSystem.getPush()};

    saveSystem.loadScene("assets/scenes/save2.json", sceneRef);
    terrainGen.createTerrain(gameObjects);
    grassRenderSystem.updateHeightMap(terrainGen.getHeightMap(), terrainGen.config.heightScale);

    auto currentTime = std::chrono::high_resolution_clock::now();
    float totalTime = 0.f;

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
            }

            // line below to update descriptorInfo
            GlobalUbo ubo{};

            ubo.projection = camera.getProjectionMatrix();
            ubo.view = camera.getView();
            ubo.inverseView = camera.getInverseView();

            ubo.fogColor = skyRenderSystem.getPush().horizonColor;
            ubo.fogNear = skyRenderSystem.getFog().near;
            ubo.fogFar = skyRenderSystem.getFog().far;
            ubo.time = totalTime;

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

    auto sea = MyGameObject::createGameObject();
    sea.modelFilePath = "assets/models/quad.obj";
    sea.textureFilePath = "assets/textures/water.jpg";
    sea.model = quadModel;
    sea.transform.translation = {0.f, 0.f, 0.f};
    sea.transform.scale = {100.f, 1.f, 100.f};
    sea.rigidBody = std::make_unique<RigidBodyComponent>();
    sea.rigidBody->mass = 0.0f;
    sea.rigidBody->restitution = 0.1f;
    sea.rigidBody->computeBoxInertia(sea.transform.scale * 0.5f);
    sea.texture = waterTexture;
    gameObjects.emplace(sea.getId(), std::move(sea));

    auto smooth_vase = MyGameObject::createGameObject();
    smooth_vase.modelFilePath = "assets/models/smooth_vase.obj";
    smooth_vase.model = smoothVase;
    smooth_vase.transform.translation = {-1.5f, 0.f, 0.f};
    smooth_vase.transform.scale = {5.f, 5.f, 5.f};
    gameObjects.emplace(smooth_vase.getId(), std::move(smooth_vase));

    auto flat_vase = MyGameObject::createGameObject();
    flat_vase.modelFilePath = "assets/models/flat_vase.obj";
    flat_vase.model = roughVase;
    flat_vase.transform.translation = {1.5f, 0.f, 0.f};
    flat_vase.transform.scale = {5.f, 5.f, 5.f};
    gameObjects.emplace(flat_vase.getId(), std::move(flat_vase));

    auto sphere = MyGameObject::createGameObject();
    sphere.modelFilePath = "assets/models/sphere.obj";
    sphere.model = sphereModel;
    sphere.transform.translation = {1.5f, -4.f, 2.f};
    sphere.transform.scale = {5.f, 5.f, 5.f};
    gameObjects.emplace(sphere.getId(), std::move(sphere));

    auto cube1 = MyGameObject::createGameObject();
    cube1.modelFilePath = "assets/models/colored_cube.obj";
    cube1.model = cubeModel;
    cube1.transform.translation = {-0.5f, -3.f, 1.f};
    cube1.transform.scale = {0.5f, 0.5f, 0.5f};
    cube1.rigidBody = std::make_unique<RigidBodyComponent>();
    cube1.rigidBody->mass = 1.0f;
    cube1.rigidBody->restitution = 0.1f;
    cube1.rigidBody->computeBoxInertia(cube1.transform.scale * 0.5f);
    gameObjects.emplace(cube1.getId(), std::move(cube1));

    auto cube2 = MyGameObject::createGameObject();
    cube2.modelFilePath = "assets/models/colored_cube.obj";
    cube2.model = cubeModel;
    cube2.transform.translation = {0.5f, -15.f, 1.f};
    cube2.transform.scale = {0.5f, 0.5f, 0.5f};
    cube2.rigidBody = std::make_unique<RigidBodyComponent>();
    cube2.rigidBody->mass = 2.0f;
    cube2.rigidBody->restitution = 0.1f;
    cube2.rigidBody->computeBoxInertia(cube2.transform.scale * 0.5f);
    gameObjects.emplace(cube2.getId(), std::move(cube2));

    auto sun = MyGameObject::createPointLight(2500000.f, 8.f, {1.f, 0.95f, 0.8f});
    sun.transform.translation = {256.f, -2000.f, 256.f};
    gameObjects.emplace(sun.getId(), std::move(sun));

    std::vector<glm::vec3> lightColors{{1.f, .1f, .1f}, {.1f, .1f, 1.f}, {.1f, 1.f, .1f},
                                       {1.f, 1.f, .1f}, {.1f, 1.f, 1.f}, {1.f, 1.f, 1.f}};

    for (int i = 0; i < lightColors.size(); i++) {
        auto pointLight = MyGameObject::createPointLight(0.2f);
        pointLight.color = lightColors[i];
        auto rotateLight =
            glm::rotate(glm::mat4(1.f), (i * glm::two_pi<float>()) / lightColors.size(), {0.f, -1.f, 0.f});
        pointLight.transform.translation = glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, 1.f));
        gameObjects.emplace(pointLight.getId(), std::move(pointLight));
    }
}

} // namespace my
