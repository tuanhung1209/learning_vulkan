#pragma once

#include "vulkan_core/device.hpp"
#include <string>
#include <vector>

namespace my{
struct GraphicPipelineConfigInfo {
	GraphicPipelineConfigInfo() = default;
	GraphicPipelineConfigInfo(const GraphicPipelineConfigInfo&) = delete;
	GraphicPipelineConfigInfo& operator=(const GraphicPipelineConfigInfo&) = delete;

	std::vector<VkVertexInputBindingDescription> bindingDescription{};
	std::vector<VkVertexInputAttributeDescription> attributeDescription{};
	VkPipelineViewportStateCreateInfo viewportInfo;
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
	VkPipelineRasterizationStateCreateInfo rasterizationInfo;
	VkPipelineMultisampleStateCreateInfo multisampleInfo;
	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	VkPipelineColorBlendStateCreateInfo colorBlendInfo;
	VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
	std::vector<VkDynamicState> dynamicStateEnables;
	VkPipelineDynamicStateCreateInfo dynamicStateInfo;
	VkPipelineLayout pipelineLayout = nullptr;
	VkRenderPass renderPass = nullptr;
	uint32_t subpass = 0;
};

class GraphicPipeline
{
public:
    GraphicPipeline(Device& device,const std::string& vertFilepath, const std::string& fragFilepath, const GraphicPipelineConfigInfo& configInfo);
    ~GraphicPipeline();

    GraphicPipeline(const GraphicPipeline&) = delete;
    GraphicPipeline& operator = (const GraphicPipeline&) = delete;

    static void defaultPipelineConfigInfo(GraphicPipelineConfigInfo& configInfo);

    void bind(VkCommandBuffer commandBuffer);
private:
	static std::vector<char> readFile(const std::string &filepath);

	void createGraphicsPipeline(const std::string &vertFilepath, const std::string &fragFilepath, const GraphicPipelineConfigInfo &configInfo);

	void createShaderModule(const std::vector<char> &code, VkShaderModule *shaderModule);

    Device &device;
    VkPipeline graphicsPipeline;
    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;
};

}