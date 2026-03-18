#include "render_systems/sky_render_system.hpp"

#include <array>
#include <memory>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
namespace my {

SkyRenderSystem::SkyRenderSystem(Device &device, VkRenderPass renderPass,
                                 VkDescriptorSetLayout globalSetLayout)
    : myDevice{device} {
    skyModel = MyModel::createModelFromFile(myDevice, "assets/models/sphere.obj");
    creatSkyTexturePoolAndSetLayout();
    createPipelineLayout(globalSetLayout);
    createPipeline(renderPass);
}

void SkyRenderSystem::creatSkyTexturePoolAndSetLayout() {
    skyTexturePool = MyDescriptorPool::Builder(myDevice)
                         .setMaxSets(1)
                         .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
                         .build();

    skyTextureSetLayout =
        MyDescriptorSetLayout::Builder(myDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

    skyTexture = std::make_shared<MyTexture>(myDevice, "assets/textures/sky_texture.png");
    skyDescriptorSet = createSkyDescriptorSet(*skyTexture);
}

VkDescriptorSet SkyRenderSystem ::createSkyDescriptorSet(MyTexture &tex) {
    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = tex.getTextureSampler();
    imageInfo.imageView = tex.getTextureImageView();
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkDescriptorSet descriptorSet;
    MyDescriptorWriter(*skyTextureSetLayout, *skyTexturePool).writeImage(0, &imageInfo).build(descriptorSet);

    return descriptorSet;
}

SkyRenderSystem::~SkyRenderSystem() { vkDestroyPipelineLayout(myDevice.device(), pipelineLayout, nullptr); }

void SkyRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout,
                                                            skyTextureSetLayout->getDescriptorSetLayout()};

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();

    if (vkCreatePipelineLayout(myDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
        VK_SUCCESS) {
        throw std::runtime_error("can not create pipelinelayout");
    }
}

void SkyRenderSystem::createPipeline(VkRenderPass renderPass) {
    assert(pipelineLayout != nullptr && "cannot create pipeline before pipeline layout");

    PipelineConfigInfo pipelineConfig{};
    PipeLine::defaultPipelineConfigInfo(pipelineConfig);

    pipelineConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;
    pipelineConfig.depthStencilInfo.depthTestEnable = VK_FALSE;
    pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;

    pipelineConfig.renderPass = renderPass;
    pipelineConfig.pipelineLayout = pipelineLayout;
    myPipeLine = std::make_unique<PipeLine>(myDevice, "shaders/sky_shader.vert.spv",
                                            "shaders/sky_shader.frag.spv", pipelineConfig);
}

void SkyRenderSystem::renderSky(FrameInfo &frameInfo) {
    myPipeLine->bind(frameInfo.commandBuffer);

    vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                            &frameInfo.globalDescriptorSet, 0, nullptr);

    vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1,
                            &skyDescriptorSet, 0, nullptr);

    skyModel->bind(frameInfo.commandBuffer);
    skyModel->draw(frameInfo.commandBuffer);
}

} // namespace my
