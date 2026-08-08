#include "render_systems/simple_render_system.hpp"

#include "vulkan_core/device.hpp"
#include "render_core/my_texture.hpp"
#include "render_core/my_model.hpp" // IWYU pragma: keep
#include "vulkan_core/graphic_pipeline.hpp"

#include "ecs/components/transform_component.hpp"
#include "ecs/components/model_component.hpp"
#include "ecs/components/texture_component.hpp"

#include <stdexcept>

namespace my {

SimpleRenderSystem::SimpleRenderSystem(Device &device, AssetCache &assetCache, VkRenderPass renderPass,
                                       VkDescriptorSetLayout globalSetLayout)
    : myDevice_{device}, assetCache_(assetCache) {
    createTexturePoolAndSetLayout();
    createGraphicPipelineLayout(globalSetLayout);
    createGraphicPipeline(renderPass);
}

SimpleRenderSystem::~SimpleRenderSystem() {
    vkDestroyPipelineLayout(myDevice_.device(), graphicPipelineLayout, nullptr);
}

void SimpleRenderSystem::createTexturePoolAndSetLayout() {
    texturePool = MyDescriptorPool::Builder(myDevice_)
                      .setMaxSets(100)
                      .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100)
                      .build();

    textureSetLayout =
        MyDescriptorSetLayout::Builder(myDevice_)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

    defaultWhiteTexture = std::make_shared<MyTexture>(myDevice_, "assets/textures/white.png");
}

void SimpleRenderSystem::createGraphicPipelineLayout(VkDescriptorSetLayout globalSetLayout) {
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

    if (vkCreatePipelineLayout(myDevice_.device(), &pipelineLayoutInfo, nullptr, &graphicPipelineLayout) !=
        VK_SUCCESS) {
        throw std::runtime_error("can not create pipelinelayout");
    }
}

void SimpleRenderSystem::createGraphicPipeline(VkRenderPass renderPass) {
    assert(graphicPipelineLayout != nullptr && "cannot create pipeline before pipeline layout");

    GraphicPipelineConfigInfo pipelineConfig{};
    GraphicPipeline::defaultPipelineConfigInfo(pipelineConfig);
    pipelineConfig.renderPass = renderPass;
    pipelineConfig.pipelineLayout = graphicPipelineLayout;
    myGraphicPipeline = std::make_unique<GraphicPipeline>(myDevice_, "shaders/simple_shader.vert.spv",
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
    myGraphicPipeline->bind(frameInfo.commandBuffer);

    vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicPipelineLayout,
                            0, 1, &frameInfo.globalDescriptorSet, 0, nullptr);

    for (auto [e, trans, mo, te] :
         frameInfo.ecsManager.query<TransformComponent, ModelComponent, TextureComponent>()) {
        if (mo.modelPath.empty()) continue;

        MyTexture &tex =
            !te.texturePath.empty() ? *assetCache_.getTexture(te.texturePath) : *defaultWhiteTexture;
        VkDescriptorSet texDescriptorSet = getOrCreateTextureDescriptorSet(tex);
        vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                graphicPipelineLayout, 1, 1, &texDescriptorSet, 0, nullptr);

        SimplePushConstantData push{};
        push.modelMatrix = trans.mat4();
        push.normalMatrix = trans.normalMatrix();

        vkCmdPushConstants(frameInfo.commandBuffer, graphicPipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                           sizeof(SimplePushConstantData), &push);

        auto model = assetCache_.getModel(mo.modelPath);
        model->bind(frameInfo.commandBuffer);
        model->draw(frameInfo.commandBuffer);
    }
}

} // namespace my
