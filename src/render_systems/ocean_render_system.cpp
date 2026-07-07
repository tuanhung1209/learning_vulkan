#include "render_systems/ocean_render_system.hpp"

#include "vulkan_core/device.hpp"
#include "vulkan_core/my_buffer.hpp"
#include "render_core/my_model.hpp"
#include "render_core/my_texture.hpp"
#include "vulkan_core/graphic_pipeline.hpp"

#include "vulkan_core/swap_chain.hpp"
#include "render_core/my_imgui.hpp"

#include <stdexcept>

namespace my {

OceanRenderSystem::OceanRenderSystem(Device &device, VkRenderPass renderPass,
                                     VkDescriptorSetLayout globalSetLayout)
    : myDevice{device} {
    oceanPlaneModel = MyModel::createModelFromFile(myDevice, "assets/models/ocean_plane.obj");
    oceanPlaneTexture = std::make_shared<MyTexture>(myDevice, "assets/textures/sky_texture.png");
    createOceanUboBuffer();
    createOceanUboPoolAndSetLayout();
    createOceanTexturePoolAndSetLayout();
    createGraphicPipelineLayout(globalSetLayout);
    createGraphicPipeline(renderPass);
}

OceanRenderSystem::~OceanRenderSystem() {
    vkDestroyPipelineLayout(myDevice.device(), graphicPipelineLayout, nullptr);
}

void OceanRenderSystem::createOceanUboBuffer() {
    uboBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < uboBuffers.size(); i++) {
        uboBuffers[i] = std::make_unique<MyBuffer>(
            myDevice, sizeof(OceanUbo), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        uboBuffers[i]->map();
    }
}

void OceanRenderSystem::createOceanUboPoolAndSetLayout() {
    oceanUboPool = MyDescriptorPool::Builder(myDevice)
                       .setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT)
                       .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SwapChain::MAX_FRAMES_IN_FLIGHT)
                       .build();

    oceanUboSetLayout = MyDescriptorSetLayout::Builder(myDevice)
                            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
                            .build();

    oceanUboDescriptorSet.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < oceanUboDescriptorSet.size(); i++) {
        auto oceanBufferInfo = uboBuffers[i]->descriptorInfo();
        MyDescriptorWriter(*oceanUboSetLayout, *oceanUboPool)
            .writeBuffer(0, &oceanBufferInfo)
            .build(oceanUboDescriptorSet[i]);
    }
}

void OceanRenderSystem::createOceanTexturePoolAndSetLayout() {
    oceanTexturePool = MyDescriptorPool::Builder(myDevice)
                           .setMaxSets(1)
                           .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
                           .build();

    oceanTextureSetLayout =
        MyDescriptorSetLayout::Builder(myDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

    oceanTextureDescriptorSet = createOceanTextureDescriptorSet(*oceanPlaneTexture);
}

VkDescriptorSet OceanRenderSystem ::createOceanTextureDescriptorSet(MyTexture &tex) {
    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = tex.getTextureSampler();
    imageInfo.imageView = tex.getTextureImageView();
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkDescriptorSet descriptorSet;
    MyDescriptorWriter(*oceanTextureSetLayout, *oceanTexturePool)
        .writeImage(0, &imageInfo)
        .build(descriptorSet);

    return descriptorSet;
}

void OceanRenderSystem::createGraphicPipelineLayout(VkDescriptorSetLayout globalSetLayout) {
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout,
                                                            oceanUboSetLayout->getDescriptorSetLayout(),
                                                            oceanTextureSetLayout->getDescriptorSetLayout()};

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();

    if (vkCreatePipelineLayout(myDevice.device(), &pipelineLayoutInfo, nullptr, &graphicPipelineLayout) !=
        VK_SUCCESS) {
        throw std::runtime_error("can not create pipelinelayout");
    }
}

void OceanRenderSystem::createGraphicPipeline(VkRenderPass renderPass) {
    assert(graphicPipelineLayout != nullptr && "cannot create pipeline before pipeline layout");

    GraphicPipelineConfigInfo pipelineConfig{};
    GraphicPipeline::defaultPipelineConfigInfo(pipelineConfig);
    pipelineConfig.renderPass = renderPass;
    pipelineConfig.pipelineLayout = graphicPipelineLayout;
    myGraphicPipeline = std::make_unique<GraphicPipeline>(myDevice, "shaders/ocean_shader.vert.spv",
                                                          "shaders/ocean_shader.frag.spv", pipelineConfig);
}

void OceanRenderSystem::drawGui() {
    ImGui::Begin("Ocean");

    if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::ColorEdit3("Horizon Color", &oceanUbo.horizonColor.x);
        ImGui::ColorEdit3("Sky Color", &oceanUbo.skyColor.x);
        ImGui::ColorEdit3("Deep Color", &oceanUbo.deepColor.x);
    }

    if (ImGui::CollapsingHeader("Waves", ImGuiTreeNodeFlags_DefaultOpen)) {
        for (int i = 0; i < MAX_OCEAN_WAVES; i++) {
            ImGui::PushID(i);
            ImGui::Text("Wave %d", i + 1);
            ImGui::SliderFloat2("Direction", &oceanUbo.waves[i].direction.x, -1.f, 1.f);
            ImGui::SliderFloat("Frequency", &oceanUbo.waves[i].frequency, 0.02f, 65.f);
            ImGui::SliderFloat("Amplitude", &oceanUbo.waves[i].amplitude, 0.f, 4.f);
            ImGui::SliderFloat("Steepness", &oceanUbo.waves[i].steepness, 0.f, 0.99f);
            ImGui::SliderFloat("Speed", &oceanUbo.waves[i].speed, 0.1f, 3.f);
            ImGui::PopID();
        }
    }

    ImGui::End();
}

void OceanRenderSystem::renderOcean(FrameInfo &frameInfo) {
    uboBuffers[frameInfo.frameIndex]->writeToBuffer(&oceanUbo);
    myGraphicPipeline->bind(frameInfo.commandBuffer);

    vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicPipelineLayout,
                            0, 1, &frameInfo.globalDescriptorSet, 0, nullptr);

    vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicPipelineLayout,
                            1, 1, &oceanUboDescriptorSet[frameInfo.frameIndex], 0, nullptr);

    vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicPipelineLayout,
                            2, 1, &oceanTextureDescriptorSet, 0, nullptr);

    oceanPlaneModel->bind(frameInfo.commandBuffer);
    oceanPlaneModel->draw(frameInfo.commandBuffer);
}

} // namespace my
