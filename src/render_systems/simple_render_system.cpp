#include "render_systems/simple_render_system.hpp"
#include "vulkan_core/my_descriptors.hpp"
#include "vulkan_core/swap_chain.hpp"
#include <GLFW/glfw3.h>

#include <array>
#include <memory>
#include <stdexcept>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
namespace my {

struct SimplePushConstantData {
    glm::mat4 modelMatrix{1.f};
    glm::mat4 normalMatrix{1.f};
};

SimpleRenderSystem::SimpleRenderSystem(Device &device, VkRenderPass renderPass,
                                       VkDescriptorSetLayout globalSetLayout)
    : myDevice{device} {
    createTexturePoolAndSetLayout();
    createPipelineLayout(globalSetLayout);
    createPipeline(renderPass);
}

SimpleRenderSystem::~SimpleRenderSystem() {
    vkDestroyPipelineLayout(myDevice.device(), pipelineLayout, nullptr);
}

void SimpleRenderSystem::createTexturePoolAndSetLayout() {
    texturePool = MyDescriptorPool::Builder(myDevice)
                      .setMaxSets(100)
                      .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100)
                      .build();

    textureSetLayout =
        MyDescriptorSetLayout::Builder(myDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

    defaultWhiteTexture = std::make_shared<MyTexture>(myDevice, "assets/textures/white.png");
}

void SimpleRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(SimplePushConstantData);

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout,
                                                            textureSetLayout->getDescriptorSetLayout()};

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(myDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
        VK_SUCCESS) {
        throw std::runtime_error("can not create pipelinelayout");
    }
}

void SimpleRenderSystem::createPipeline(VkRenderPass renderPass) {
    assert(pipelineLayout != nullptr && "cannot create pipeline before pipeline layout");

    PipelineConfigInfo pipelineConfig{};
    PipeLine::defaultPipelineConfigInfo(pipelineConfig);
    pipelineConfig.renderPass = renderPass;
    pipelineConfig.pipelineLayout = pipelineLayout;
    myPipeLine = std::make_unique<PipeLine>(myDevice, "shaders/simple_shader.vert.spv",
                                            "shaders/simple_shader.frag.spv", pipelineConfig);
}

VkDescriptorSet SimpleRenderSystem::getOrCreateTextureDescriptorSet(MyTexture &tex) {
    // the pointer situation is bad
    auto it = textureDescriptorSets.find(&tex);
    if (it != textureDescriptorSets.end()) return it->second;

    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = tex.getTextureSampler();
    imageInfo.imageView = tex.getTextureImageView();
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkDescriptorSet descriptorSet;
    MyDescriptorWriter(*textureSetLayout, *texturePool).writeImage(0, &imageInfo).build(descriptorSet);

    textureDescriptorSets[&tex] = descriptorSet;
    return descriptorSet;
}

void SimpleRenderSystem::renderGameObjects(FrameInfo &frameInfo) {
    myPipeLine->bind(frameInfo.commandBuffer);

    vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                            &frameInfo.globalDescriptorSet, 0, nullptr);

    // can split into multiple vector to have object with different component/attribute
    for (auto &kv : frameInfo.gameObjecs) {
        auto &obj = kv.second;

        if (obj.model == nullptr) continue;

        MyTexture &tex = obj.texture ? *obj.texture : *defaultWhiteTexture;
        VkDescriptorSet texDescriptorSet = getOrCreateTextureDescriptorSet(tex);
        vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1,
                                1, &texDescriptorSet, 0, nullptr);

        SimplePushConstantData push{};
        push.modelMatrix = obj.transform.mat4();
        push.normalMatrix = obj.transform.normalMatrix();

        vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                           sizeof(SimplePushConstantData), &push);

        obj.model->bind(frameInfo.commandBuffer);
        obj.model->draw(frameInfo.commandBuffer);
    }
}

} // namespace my
