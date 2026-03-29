#pragma once

#include "vulkan_core/device.hpp"
#include <string>
#include <vulkan/vulkan_core.h>

namespace my {
class ComputePipeline {
  public:
    ComputePipeline(Device &device, const std::string &shaderFilepath, VkPipelineLayout &layout);
    ~ComputePipeline();

    ComputePipeline(const ComputePipeline &) = delete;
    ComputePipeline &operator=(const ComputePipeline &) = delete;

    void bind(VkCommandBuffer commandBuffer);

  private:
    static std::vector<char> readFile(const std::string &filepath);

    void createComputePipeline(const std::string &shaderFilepath, VkPipelineLayout &layout);
    void createComputeShaderModule(const std::vector<char> &code, VkShaderModule *shaderModule);

    Device &device;
    VkPipeline computePipeline;
    VkShaderModule computeShaderModule;
};

} // namespace my
