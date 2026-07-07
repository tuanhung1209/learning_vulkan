#include "game/first_app.hpp"

#include "game/my_player.hpp"
#include "game/terrain_generation.hpp"
#include "game/scene_file_panel.hpp"
#include "game/scene_reference.hpp"
#include "game/sun_panel.hpp"
#include "game/gui_manager.hpp"
#include "game/my_save_system.hpp"
#include "vulkan_core/my_buffer.hpp"
#include "input/glfw_input_bridge.hpp"

#include "render_core/my_model.hpp"
#include "render_core/my_imgui.hpp"
#include "render_systems/grass_render_system.hpp"
#include "render_systems/ocean_render_system.hpp"
#include "render_systems/sky_render_system.hpp"
#include "render_systems/point_light_system.hpp"
#include "render_systems/simple_render_system.hpp"

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

    GuiManager guiManager{};
    guiManager.registerGuiPanel(std::make_unique<SunPanel>());

    std::shared_ptr<MyModel> bulletModel = MyModel::createModelFromFile(*device, "assets/models/cube.obj");
    BulletHandler bulletHandler{bulletModel};

    InputState inputState{};
    std::unique_ptr<GlfwInput> glfwInput{};
    if (mode_ == Mode::Edit) { glfwInput = std::make_unique<GlfwInput>(*glfwWindow, inputState); }

    MyCamera camera{};
    camera.setViewTarget(glm::vec3(-1.f, -2.f, 2.f), glm::vec3(0.f, 0.f, 2.5f));

    auto playerObject = MyGameObject::createGameObject();
    auto playerId = playerObject.getId();
    gameObjects.emplace(playerId, std::move(playerObject));
    MyPlayer mainPlayer{camera, playerId, inputState};

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

        float aspect = myRenderer->getAspectRatio();
        camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 10000.f);

        if (auto commandBuffer = myRenderer->beginFrame()) {
            int frameIndex = myRenderer->getFrameIndex();
            FrameInfo frameInfo{frameIndex, frameTime, commandBuffer, camera, globalDescriptorSet[frameIndex],
                                gameObjects};

            guiManager.updatePanels(sceneRef);

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

                guiManager.drawGuiPanel(sceneRef);
            }

            // line below to update GlobalUbo
            GlobalUbo ubo{};

            ubo.projection = camera.getProjectionMatrix();
            ubo.view = camera.getView();
            ubo.inverseView = camera.getInverseView();

            ubo.fogColor = skyRenderSystem.getFog().fogColor;
            ubo.fogNear = skyRenderSystem.getFog().near;
            ubo.fogFar = skyRenderSystem.getFog().far;
            ubo.fogDensity = skyRenderSystem.getFog().density;

            ubo.time = totalTime;

            ubo.sunDirection = sceneRef.sunDirection;

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

void FirstApp::loadGameObjects() {}

} // namespace my
