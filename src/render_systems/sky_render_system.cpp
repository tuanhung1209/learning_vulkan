#include "render_systems/sky_render_system.hpp"
#include "render_core/my_frame_info.hpp"
#include "vulkan_core/my_descriptors.hpp"
#include "vulkan_core/swap_chain.hpp"

#include <array>
#include <imgui.h>
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
    createSkyTexturePoolAndSetLayout();
    createSkyUboPoolAndSetLayout();
    createPipelineLayout(globalSetLayout);
    createPipeline(renderPass);
}
SkyRenderSystem::~SkyRenderSystem() { vkDestroyPipelineLayout(myDevice.device(), pipelineLayout, nullptr); }

void SkyRenderSystem::createSkyUboPoolAndSetLayout() {
    skyUboBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < skyUboBuffers.size(); i++) {
        skyUboBuffers[i] =
            std::make_unique<MyBuffer>(myDevice, sizeof(SkyUbo), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        skyUboBuffers[i]->map();
    }

    skyUboPool = MyDescriptorPool::Builder(myDevice)
                     .setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT)
                     .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SwapChain::MAX_FRAMES_IN_FLIGHT)
                     .build();

    skyUboSetLayout = MyDescriptorSetLayout::Builder(myDevice)
                          .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
                          .build();

    skyUboDescriptorSet.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < skyUboDescriptorSet.size(); i++) {
        auto bufferInfo = skyUboBuffers[i]->descriptorInfo();
        MyDescriptorWriter(*skyUboSetLayout, *skyUboPool)
            .writeBuffer(0, &bufferInfo)
            .build(skyUboDescriptorSet[i]);
    }
}

void SkyRenderSystem::createSkyTexturePoolAndSetLayout() {
    skyTexturePool = MyDescriptorPool::Builder(myDevice)
                         .setMaxSets(1)
                         .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
                         .build();

    skyTextureSetLayout =
        MyDescriptorSetLayout::Builder(myDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

    skyTexture = std::make_shared<MyTexture>(myDevice, "assets/textures/sky_texture.png");
    skyTextureDescriptorSet = createSkyDescriptorSet(*skyTexture);
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

void SkyRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout,
                                                            skyTextureSetLayout->getDescriptorSetLayout(),
                                                            skyUboSetLayout->getDescriptorSetLayout()};

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

void SkyRenderSystem::updateUbo(FrameInfo &frameInfo, SkyUbo &skyUbo) {
    ImGui::Begin("Sky");
    ImGui::ColorPicker4("Horizon Color", &skyUbo.horizonColor.x);
    ImGui::ColorPicker4("Sky Color", &skyUbo.skyColor.x);
    ImGui::ColorPicker4("Sky TColor", &skyUbo.skyTextureColor.x);
    ImGui::SliderFloat3("Sun Direction", &skyUbo.sunDirection.x, -1.f, 1.f);
    ImGui::SliderFloat("Cloud Speed", &skyUbo.time, -1.0f, 1.0f);
    ImGui::End();
    skyUboBuffers[frameInfo.frameIndex]->writeToBuffer(&skyUbo);
    skyUboBuffers[frameInfo.frameIndex]->flush();
}

void SkyRenderSystem::renderSky(FrameInfo &frameInfo) {
    myPipeLine->bind(frameInfo.commandBuffer);

    vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                            &frameInfo.globalDescriptorSet, 0, nullptr);

    vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1,
                            &skyTextureDescriptorSet, 0, nullptr);

    vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, 1,
                            &skyUboDescriptorSet[frameInfo.frameIndex], 0, nullptr);

    skyModel->bind(frameInfo.commandBuffer);
    skyModel->draw(frameInfo.commandBuffer);
}

} // namespace my
