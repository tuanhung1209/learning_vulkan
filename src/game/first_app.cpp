#include "game/first_app.hpp"
#include "render_systems/simple_render_system.hpp"
#include "render_systems/point_light_system.hpp"
#include "game/keyboard_movement_controller.hpp"
#include "vulkan_core/my_buffer.hpp"
#include "game/my_Player.hpp"
#include "game/physics_utils.hpp"

#include <chrono>
#include <memory>
#include <cstdio>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
namespace my {

FirstApp::FirstApp() {
    globalPool = MyDescriptorPool::Builder(device)
                     .setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT)
                     .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SwapChain::MAX_FRAMES_IN_FLIGHT)
                     .build();

    loadGameObjects();
}

FirstApp::~FirstApp() {}

void FirstApp::run() {
    std::vector<std::unique_ptr<MyBuffer>> uboBuffers(SwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < uboBuffers.size(); i++) {
        uboBuffers[i] = std::make_unique<MyBuffer>(device, sizeof(GlobalUbo), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        uboBuffers[i]->map();
    }

    auto globalSetLayout = MyDescriptorSetLayout::Builder(device)
                               .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
                               .build();

    std::vector<VkDescriptorSet> globalDescriptorSet(SwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < globalDescriptorSet.size(); i++) {
        auto bufferInfo = uboBuffers[i]->descriptorInfo();
        MyDescriptorWriter(*globalSetLayout, *globalPool)
            .writeBuffer(0, &bufferInfo)
            .build(globalDescriptorSet[i]);
    }

    SimpleRenderSystem simpleRenderSystem{device, myRenderer.getSwapChainRenderPass(),
                                          globalSetLayout->getDescriptorSetLayout()};
    PointLightSystem PointLightSystem{device, myRenderer.getSwapChainRenderPass(),
                                      globalSetLayout->getDescriptorSetLayout()};
    MyCamera camera{};

    // camera.setViewDirection(glm::vec3(0.f), glm::vec3(0.7f, 0.f, 1.f));
    camera.setViewTarget(glm::vec3(-1.f, -2.f, 2.f), glm::vec3(0.f, 0.f, 2.5f));
    
    // Initialize Bullet Handler
    std::shared_ptr<MyModel> bulletModel = MyModel::createModelFromFile(device, "models/cube.obj");
    BulletHandler bulletHandler{bulletModel};

    //TODO : init an id in the main player file
    auto playerObject = MyGameObject::createGameObject();
    playerObject.transform.translation.z = -2.5f;

    MyPlayer mainPlayer{camera, playerObject.getId()};
    
    // TODO : test this later for camera position
    
    gameObjects.emplace(playerObject.getId(), std::move(playerObject));

    long long health = 10000;
    auto currentTime = std::chrono::high_resolution_clock::now();

    while (!window.shouldClose()) {
        glfwPollEvents();

        auto newTime = std::chrono::high_resolution_clock::now();
        auto frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;

        // may add smallest time frame to prevent frame skipping

        mainPlayer.update(window.getWindow(), frameTime, gameObjects, bulletHandler);
        bulletHandler.update(frameTime);

        float aspect = myRenderer.getAspectRatio();
        //camera.setOrthographicProjection(-aspect, aspect, -1, 1, -1, 1);
        camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 100.f);

        if (auto commandBuffer = myRenderer.beginFrame()) {
            int frameIndex = myRenderer.getFrameIndex();
            FrameInfo frameInfo{frameIndex, frameTime, commandBuffer, camera, globalDescriptorSet[frameIndex],
                                gameObjects};

            // line below to update
            GlobalUbo ubo{};
            ubo.projection = camera.getProjectionMatrix();
            ubo.view = camera.getView();
            PointLightSystem.update(frameInfo, ubo);
            uboBuffers[frameIndex]->writeToBuffer(&ubo);
            uboBuffers[frameIndex]->flush();

            // line below to render
            myRenderer.beginSwapChainRenderPass(commandBuffer);
            simpleRenderSystem.renderGameObjects(frameInfo);
            
            // Render Bullets
            bulletHandler.renderBullet(commandBuffer, simpleRenderSystem.getPipelineLayout());

            PointLightSystem.renderLight(frameInfo);

            myRenderer.endSwapChainRenderPass(commandBuffer);
            myRenderer.endFrame();
        }
    }
    vkDeviceWaitIdle(device.device());
}

void FirstApp::loadGameObjects() {
    std::shared_ptr<MyModel> gameModel = MyModel::createModelFromFile(device, "models/smooth_vase.obj");
    auto smoothVase = MyGameObject::createGameObject();
    smoothVase.model = gameModel;
    smoothVase.transform.translation = {-.5f, .5f, .0f};
    smoothVase.transform.scale = glm::vec3(3.f);
    gameObjects.emplace(smoothVase.getId(), std::move(smoothVase));

    std::shared_ptr<MyModel> gameModel1 = MyModel::createModelFromFile(device, "models/flat_vase.obj");
    auto flatVase = MyGameObject::createGameObject();
    flatVase.model = gameModel1;
    flatVase.transform.translation = {.5f, .5f, .0f};
    flatVase.transform.scale = glm::vec3(3.f);
    gameObjects.emplace(flatVase.getId(), std::move(flatVase));

    std::shared_ptr<MyModel> gameModel2 = MyModel::createModelFromFile(device, "models/quad.obj");
    auto quad = MyGameObject::createGameObject();
    quad.model = gameModel2;
    quad.transform.translation = {.0f, .5f, .0f};
    quad.transform.scale = glm::vec3(3.f);
    gameObjects.emplace(quad.getId(),std::move(quad));

    // Target Box (This will be ID 3)
    std::shared_ptr<MyModel> cubeModel = MyModel::createModelFromFile(device, "models/colored_cube.obj");
    auto cube = MyGameObject::createGameObject();
    cube.model = cubeModel;
    cube.transform.translation = {1.5f, .5f, .0f};
    cube.transform.scale = {0.5f, 0.5f, 0.5f};
    gameObjects.emplace(cube.getId(), std::move(cube));

    std::vector<glm::vec3> lightColors{
        {1.f, .1f, .1f},
        {.1f, .1f, 1.f},
        {.1f, 1.f, .1f},
        {1.f, 1.f, .1f},
        {.1f, 1.f, 1.f},
        {1.f, 1.f, 1.f} 
    };

    for (int i = 0; i < lightColors.size(); i++){
        auto pointLight = MyGameObject::createPointLight(0.2f);
        pointLight.color = lightColors[i];
        auto rotateLight = glm::rotate(glm::mat4(1.f), (i * glm::two_pi<float>()) / lightColors.size(),
                                        {0.f, -1.f, 0.f});
        pointLight.transform.translation = glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, 1.f));
        gameObjects.emplace(pointLight.getId(), std::move(pointLight));
    }
}

}