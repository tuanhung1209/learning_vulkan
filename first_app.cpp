#include <GLFW/glfw3.h>
#include "first_app.hpp"
#include "systems/simple_render_system.hpp"
#include "systems/point_light_system.hpp"
#include "keyboard_movement_controller.hpp"
#include "my_buffer.hpp"
#include "my_Player.hpp"

#include <chrono>
#include <stdexcept>
#include <array>
#include <memory>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
namespace my{

struct GlobalUbo{
    glm::mat4 projection{1.f};
    glm::mat4 view{1.f};
    glm::vec4 ambientLightColor{1.f, 1.f, 1.f, .02f};
    glm::vec3 lightPosition{-1.f};
    alignas(16) glm::vec4 lightColor{1.f, 0.9f, 0.6f, 1.f};
};

FirstApp::FirstApp(){
    globalPool = MyDescriptorPool::Builder(device)
    .setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT)
    .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SwapChain::MAX_FRAMES_IN_FLIGHT)
    .build();

    loadGameObjects();
}

FirstApp::~FirstApp() {}

void FirstApp::run() {
    std::vector<std::unique_ptr<MyBuffer>> uboBuffers(SwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < uboBuffers.size(); i++){
        uboBuffers[i] = std::make_unique<MyBuffer>(device,
        sizeof(GlobalUbo), 
        1,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        uboBuffers[i]->map();
    }

    auto globalSetLayout = MyDescriptorSetLayout::Builder(device).addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS).build();
    
    std::vector<VkDescriptorSet> globalDescriptorSet(SwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < globalDescriptorSet.size(); i++){
        auto bufferInfo = uboBuffers[i]->descriptorInfo();
        MyDescriptorWriter(*globalSetLayout, *globalPool).writeBuffer(0, &bufferInfo).build(globalDescriptorSet[i]);
    }

    SimpleRenderSystem simpleRenderSystem{device, myRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout()};
    PointLightSystem PointLightSystem{device, myRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout()};
    MyCamera camera{};

    // camera.setViewDirection(glm::vec3(0.f), glm::vec3(0.7f, 0.f, 1.f));
    camera.setViewTarget(glm::vec3(-1.f, -2.f, 2.f), glm::vec3(0.f, 0.f, 2.5f));

    //std::shared_ptr<MyModel> cubeModel = MyModel::createModelFromFile(device, "models/cube.obj");

    auto playerObject = MyGameObject::createGameObject();
    gameObjects.emplace(playerObject.getId(), std::move(playerObject));
    MyPlayer mainPlayer{MyModel::createModelFromFile(device, "models/cube.obj"), camera, playerObject.getId()};
    
    // TODO : test this later for camera position
    playerObject.transform.translation.z = -2.5f;

    auto currentTime = std::chrono::high_resolution_clock::now();
           
    while (!window.shouldClose()){
        glfwPollEvents();

        auto newTime = std::chrono::high_resolution_clock::now();
        auto frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;

        // may add smallest time frame to prevent frame skipping
        mainPlayer.update(window.getWindow(), frameTime, gameObjects);

        float aspect = myRenderer.getAspectRatio();
        //camera.setOrthographicProjection(-aspect, aspect, -1, 1, -1, 1);
        camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 100.f);

        if (auto commandBuffer = myRenderer.beginFrame()){
            int frameIndex = myRenderer.getFrameIndex();
            FrameInfo frameInfo{frameIndex, frameTime, commandBuffer, camera, globalDescriptorSet[frameIndex], gameObjects};

            // line below to update 
            GlobalUbo ubo{};
            ubo.projection = camera.getProjectionMatrix();
            ubo.view = camera.getView();
            uboBuffers[frameIndex]->writeToBuffer(&ubo);
            uboBuffers[frameIndex]->flush();

            // line below to render 
            myRenderer.beginSwapChainRenderPass(commandBuffer);
            simpleRenderSystem.renderGameObjects(frameInfo);
            PointLightSystem.renderLight(frameInfo);

            myRenderer.endSwapChainRenderPass(commandBuffer);
            myRenderer.endFrame();
        }
    }
    vkDeviceWaitIdle(device.device());
}


void FirstApp::loadGameObjects(){
    std::shared_ptr<MyModel> gameModel = MyModel::createModelFromFile(device, "models/smooth_vase.obj");
    auto smoothVase = MyGameObject::createGameObject();
    smoothVase.model = gameModel;
    smoothVase.transform.translation = {-.5f, .5f, .0f};
    smoothVase.transform.scale = glm::vec3(3.f);
    gameObjects.emplace(smoothVase.getId(),std::move(smoothVase));

    std::shared_ptr<MyModel> gameModel1 = MyModel::createModelFromFile(device, "models/flat_vase.obj");
    auto flatVase = MyGameObject::createGameObject();
    flatVase.model = gameModel1;
    flatVase.transform.translation = {.5f, .5f, .0f};
    flatVase.transform.scale = glm::vec3(3.f);
    gameObjects.emplace(flatVase.getId(),std::move(flatVase));

    std::shared_ptr<MyModel> gameModel2 = MyModel::createModelFromFile(device, "models/quad.obj");
    auto quad = MyGameObject::createGameObject();
    quad.model = gameModel2;
    quad.transform.translation = {.0f, .5f, .0f};
    quad.transform.scale = glm::vec3(3.f);
    gameObjects.emplace(quad.getId(),std::move(quad));
}

}
