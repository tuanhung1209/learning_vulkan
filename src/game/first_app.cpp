#include "game/first_app.hpp"
#include "game/keyboard_movement_controller.hpp"
#include "game/my_Player.hpp"
#include "game/physics_utils.hpp"
#include "math/perlin_noise.hpp"
#include "render_core/my_texture.hpp"
#include "render_systems/point_light_system.hpp"
#include "render_systems/simple_render_system.hpp"
#include "vulkan_core/my_buffer.hpp"
#include "vulkan_core/swap_chain.hpp"

#include <GLFW/glfw3.h>
#include <chrono>
#include <cstdio>
#include <memory>
#include <vector>

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
        uboBuffers[i] =
            std::make_unique<MyBuffer>(device, sizeof(GlobalUbo), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
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

    // TODO : init an id in the main player file
    auto playerObject = MyGameObject::createGameObject();
    playerObject.rigidBody = std::make_unique<RigidBodyComponent>();
    playerObject.rigidBody->friction = 0.2f;
    playerObject.transform.translation = glm::vec3(1.f, -10.f, 1.f);
    MyPlayer mainPlayer{camera, playerObject.getId()};
    gameObjects.emplace(playerObject.getId(), std::move(playerObject));

    PhysicsWorld physicsWorld;
    auto currentTime = std::chrono::high_resolution_clock::now();

    while (!window.shouldClose()) {
        glfwPollEvents();

        auto newTime = std::chrono::high_resolution_clock::now();
        auto frameTime =
            std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;

        frameTime = std::min(frameTime, 0.05f);

        // Update player input and bullet lifetimes
        mainPlayer.update(window.getWindow(), frameTime, gameObjects, bulletHandler);
        bulletHandler.update(frameTime);

        // Physics: fixed timestep, iterative solver, collision detection + resolution
        physicsWorld.step(gameObjects, bulletHandler.getBullets(), frameTime);

        if (glfwGetKey(window.getWindow(), GLFW_KEY_F) == GLFW_PRESS) {
            for (auto &kv : gameObjects) {
                if (kv.second.rigidBody && kv.second.rigidBody->mass == 2.0f) {
                    kv.second.rigidBody->velocity.x = -5.0f;
                }
            }
        }

        float aspect = myRenderer.getAspectRatio();
        // camera.setOrthographicProjection(-aspect, aspect, -1, 1, -1, 1);
        camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 100.f);

        if (auto commandBuffer = myRenderer.beginFrame()) {
            int frameIndex = myRenderer.getFrameIndex();
            FrameInfo frameInfo{frameIndex, frameTime, commandBuffer, camera, globalDescriptorSet[frameIndex],
                                gameObjects};

            // line below to update descriptorInfo
            GlobalUbo ubo{};
            ubo.projection = camera.getProjectionMatrix();
            ubo.view = camera.getView();
            PointLightSystem.update(frameInfo, ubo);
            uboBuffers[frameIndex]->writeToBuffer(&ubo);
            uboBuffers[frameIndex]->flush();

            // line below to render
            myRenderer.beginSwapChainRenderPass(commandBuffer);
            simpleRenderSystem.renderGameObjects(frameInfo);

            bulletHandler.renderBullet(commandBuffer, simpleRenderSystem.getPipelineLayout());
            PointLightSystem.renderLight(frameInfo);

            myRenderer.endSwapChainRenderPass(commandBuffer);
            myRenderer.endFrame();
        }
    }
    vkDeviceWaitIdle(device.device());
}

void FirstApp::loadGameObjects() {

    auto testTexture1 = std::make_shared<MyTexture>(device, "textures/test1.png");
    auto testTexture2 = std::make_shared<MyTexture>(device, "textures/test2.png");

    std::shared_ptr<MyModel> cubeModel = MyModel::createModelFromFile(device, "models/colored_cube.obj");
    std::shared_ptr<MyModel> quadModel = MyModel::createModelFromFile(device, "models/quad.obj");

    auto floor = MyGameObject::createGameObject();
    floor.model = quadModel;
    floor.transform.translation = {0.f, 0.5f, 0.f};
    floor.transform.scale = {10.f, 0.2f, 10.f};
    floor.rigidBody = std::make_unique<RigidBodyComponent>();
    floor.rigidBody->mass = 0.0f;
    floor.rigidBody->restitution = 0.1f;
    floor.rigidBody->computeBoxInertia(floor.transform.scale * 0.5f);
    floor.texture = testTexture1;
    gameObjects.emplace(floor.getId(), std::move(floor));

    auto perlinViewer = MyGameObject::createGameObject();
    perlinViewer.model = quadModel;
    perlinViewer.transform.translation = {4.f, -2.f, 1.f};
    perlinViewer.transform.scale = {2.f, 2.f, 2.f};
    perlinViewer.transform.rotation = {0.0f, glm::quarter_pi<float>(), glm::half_pi<float>()};

    int perlinWidth = 512;
    int perlinHeight = 512;

    const int GRID_SIZE = 200;

    std::vector<uint8_t> pixels(perlinWidth * perlinHeight * 4);
    for (int x = 0; x < perlinWidth; x++) {
        for (int y = 0; y < perlinHeight; y++) {
            int index = (y * perlinWidth + x) * 4;

            float val = 0;

            float freq = 1;
            float amp = 1;

            for (int i = 0; i < 8; i++) {
                val += PerlinGenarator::perlin(x * freq / GRID_SIZE, y * freq / GRID_SIZE) * amp;

                freq *= 2;
                amp /= 2;
            }
            val *= 1.2f;

            if (val > 1.0f) {
                val = 1.0f;
            } else if (val < -1.0f) {
                val = -1.0f;
            }

            int color = (int)(((val + 1.0f) * 0.5f) * 255);

            pixels[index] = color;
            pixels[index + 1] = color;
            pixels[index + 2] = color;
            pixels[index + 3] = 255;
        }
    }

    auto perlinTexture = std::make_shared<MyTexture>(device, perlinWidth, perlinHeight, pixels);
    perlinViewer.texture = perlinTexture;
    gameObjects.emplace(perlinViewer.getId(), std::move(perlinViewer));

    auto cube1 = MyGameObject::createGameObject();
    cube1.model = cubeModel;
    cube1.transform.translation = {-0.5f, -3.f, 0.f};
    cube1.transform.scale = {0.5f, 0.5f, 0.5f};
    cube1.rigidBody = std::make_unique<RigidBodyComponent>();
    cube1.rigidBody->mass = 1.0f;
    cube1.rigidBody->restitution = 0.1f;
    cube1.rigidBody->computeBoxInertia(cube1.transform.scale * 0.5f);
    gameObjects.emplace(cube1.getId(), std::move(cube1));

    auto cube2 = MyGameObject::createGameObject();
    cube2.model = cubeModel;
    cube2.transform.translation = {0.5f, -15.f, 0.f};
    cube2.transform.scale = {0.5f, 0.5f, 0.5f};
    cube2.rigidBody = std::make_unique<RigidBodyComponent>();
    cube2.rigidBody->mass = 2.0f;
    cube2.rigidBody->restitution = 0.1f;
    cube2.rigidBody->computeBoxInertia(cube2.transform.scale * 0.5f);
    gameObjects.emplace(cube2.getId(), std::move(cube2));

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
