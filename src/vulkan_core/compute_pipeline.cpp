#include "vulkan_core/compute_pipeline.hpp"

#include <cassert>
#include <fstream>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace my {

ComputePipeline::ComputePipeline(Device &device, const std::string &shaderFilepath, VkPipelineLayout &layout)
    : device{device} {
    createComputePipeline(shaderFilepath, layout);
}

ComputePipeline::~ComputePipeline() {
    vkDestroyShaderModule(device.device(), computeShaderModule, nullptr);
    vkDestroyPipeline(device.device(), computePipeline, nullptr);
}

void ComputePipeline::bind(VkCommandBuffer commandBuffer) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
}

std::vector<char> ComputePipeline::readFile(const std::string &filepath) {

    std::ifstream file(filepath, std::ios::ate | std::ios::binary);

    if (!file.is_open()) { throw std::runtime_error("faid to open file: " + filepath); }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    return buffer;
}

void ComputePipeline::createComputePipeline(const std::string &shaderFilepath, VkPipelineLayout &layout) {

    assert(layout != VK_NULL_HANDLE && "no pipelinelayout have been provied");

    auto computeCode = readFile(shaderFilepath);
    createComputeShaderModule(computeCode, &computeShaderModule);

    VkPipelineShaderStageCreateInfo shaderStage{};
    shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStage.module = computeShaderModule;
    shaderStage.pName = "main";

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = shaderStage;
    pipelineInfo.layout = layout;

    if (vkCreateComputePipelines(device.device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                                 &computePipeline) != VK_SUCCESS) {
        throw std::runtime_error("unable to create compute pipeline");
    }
}

void ComputePipeline::createComputeShaderModule(const std::vector<char> &code, VkShaderModule *shaderModule) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

    if (vkCreateShaderModule(device.device(), &createInfo, nullptr, shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("fail to create compute shader");
    }
}

} // namespace my
