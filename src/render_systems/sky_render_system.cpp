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
    skyTexture = std::make_shared<MyTexture>(myDevice, "assets/textures/sky_texture.png");
    createSkyTexturePoolAndSetLayout();

    createGraphicPipelineLayout(globalSetLayout);
    createGraphicPipeline(renderPass);
}

SkyRenderSystem::~SkyRenderSystem() {
    vkDestroyPipelineLayout(myDevice.device(), graphicPipelineLayout, nullptr);
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

    skyTextureDescriptorSet = createSkyTextureDescriptorSet(*skyTexture);
}

VkDescriptorSet SkyRenderSystem ::createSkyTextureDescriptorSet(MyTexture &tex) {
    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = tex.getTextureSampler();
    imageInfo.imageView = tex.getTextureImageView();
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkDescriptorSet descriptorSet;
    MyDescriptorWriter(*skyTextureSetLayout, *skyTexturePool).writeImage(0, &imageInfo).build(descriptorSet);
    return descriptorSet;
}

void SkyRenderSystem::createGraphicPipelineLayout(VkDescriptorSetLayout globalSetLayout) {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(SkyPush);

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout,
                                                            skyTextureSetLayout->getDescriptorSetLayout()};

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(myDevice.device(), &pipelineLayoutInfo, nullptr, &graphicPipelineLayout) !=
        VK_SUCCESS) {
        throw std::runtime_error("can not create pipelinelayout");
    }
}

void SkyRenderSystem::createGraphicPipeline(VkRenderPass renderPass) {
    assert(graphicPipelineLayout != nullptr && "cannot create pipeline before pipeline layout");

    GraphicPipelineConfigInfo pipelineConfig{};
    GraphicPipeline::defaultPipelineConfigInfo(pipelineConfig);

    pipelineConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;
    pipelineConfig.depthStencilInfo.depthTestEnable = VK_FALSE;
    pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;

    pipelineConfig.renderPass = renderPass;
    pipelineConfig.pipelineLayout = graphicPipelineLayout;
    myGraphicPipeline = std::make_unique<GraphicPipeline>(myDevice, "shaders/sky_shader.vert.spv",
                                                          "shaders/sky_shader.frag.spv", pipelineConfig);
}

void SkyRenderSystem::drawGui() {
    ImGui::Begin("Sky");
    ImGui::ColorPicker4("Horizon Color", &push.horizonColor.x);
    ImGui::ColorPicker4("Sky Color", &push.skyColor.x);
    ImGui::ColorPicker4("Sky TColor", &push.skyTextureColor.x);
    ImGui::SliderFloat3("Sun Direction", &push.sunDirection.x, -1.f, 1.f);
    ImGui::SeparatorText("Fog");
    ImGui::SliderFloat("Fog Near", &fog.near, 0.f, 1000.f);
    ImGui::SliderFloat("Fog Far", &fog.far, 0.f, 2000.f);
    ImGui::End();
}

void SkyRenderSystem::renderSky(FrameInfo &frameInfo) {
    myGraphicPipeline->bind(frameInfo.commandBuffer);

    vkCmdPushConstants(frameInfo.commandBuffer, graphicPipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SkyPush), &push);

    vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicPipelineLayout,
                            0, 1, &frameInfo.globalDescriptorSet, 0, nullptr);

    vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicPipelineLayout,
                            1, 1, &skyTextureDescriptorSet, 0, nullptr);

    skyModel->bind(frameInfo.commandBuffer);
    skyModel->draw(frameInfo.commandBuffer);
}

} // namespace my
