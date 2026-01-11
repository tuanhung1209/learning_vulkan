#include <GLFW/glfw3.h>
#include "render_systems/point_light_system.hpp"

#include <stdexcept>
#include <array>
#include <memory>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
namespace my{

struct PointLightPushConstants {
    glm::vec4 position{};
    glm::vec4 color{};
    float radius;
};

PointLightSystem::PointLightSystem(Device &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout) : myDevice{device}{
    createPipelineLayout(globalSetLayout);
    createPipeline(renderPass);
}

PointLightSystem::~PointLightSystem(){
    vkDestroyPipelineLayout(myDevice.device(), pipelineLayout, nullptr);
}

void PointLightSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout){
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PointLightPushConstants);

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout};
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    
    if (vkCreatePipelineLayout(myDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS){
        throw std::runtime_error("can not create pipelinelayout");
    }
}

void PointLightSystem::createPipeline(VkRenderPass renderPass){
    assert(pipelineLayout != nullptr && "cannot create pipeline before pipeline layout");

    PipelineConfigInfo pipelineConfig{};
    PipeLine::defaultPipelineConfigInfo(pipelineConfig);
    pipelineConfig.attributeDescription.clear();
    pipelineConfig.bindingDescription.clear();
    pipelineConfig.renderPass = renderPass;
    pipelineConfig.pipelineLayout = pipelineLayout;
    myPipeLine = std::make_unique<PipeLine>(myDevice, "shaders/point_light.vert.spv", "shaders/point_light.frag.spv", pipelineConfig);
}

void PointLightSystem::update(FrameInfo &frameInfo, GlobalUbo &ubo){
    auto rotateLight = glm::rotate(glm::mat4(1.f), frameInfo.frameTime, {0.f, -1.f, 0.f});
    int lightIndex = 0;
    for (auto& kv : frameInfo.gameObjecs){
        auto& obj = kv.second;
        if (obj.pointLight == nullptr) continue;

        assert(lightIndex < MAX_LIGHT && "light overflow");

        obj.transform.translation = glm::vec3(rotateLight * glm::vec4(obj.transform.translation, 1.f));

        ubo.pointLights[lightIndex].position = glm::vec4(obj.transform.translation, 1.f);
        ubo.pointLights[lightIndex].color = glm::vec4(obj.color, obj.pointLight->lightIntensity);

        lightIndex++;
    }
    ubo.numLights = lightIndex;
}

void PointLightSystem::renderLight(FrameInfo &frameInfo){
    myPipeLine->bind(frameInfo.commandBuffer);

    vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &frameInfo.globalDescriptorSet, 0, nullptr);

    for (auto& kv : frameInfo.gameObjecs){
        auto& obj = kv.second;
        if (obj.pointLight == nullptr) continue;

        PointLightPushConstants push{};
        push.position = glm::vec4(obj.transform.translation, 1.f);
        push.color = glm::vec4(obj.color, obj.pointLight->lightIntensity);
        push.radius = obj.transform.scale.x;

        vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout,
                            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                            0, sizeof(PointLightPushConstants), &push);

        vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
    }
}

}