#include "vulkan_core/my_descriptors.hpp"
 
// std
#include <cassert>
#include <stdexcept>
 
namespace my{
 
// *************** Descriptor Set Layout Builder *********************

MyDescriptorSetLayout::Builder &MyDescriptorSetLayout::Builder::addBinding(
    uint32_t binding,
    VkDescriptorType descriptorType,
    VkShaderStageFlags stageFlags,
    uint32_t count) {

    assert(bindings.count(binding) == 0 && "Binding already in use");
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = binding;
    layoutBinding.descriptorType = descriptorType;
    layoutBinding.descriptorCount = count;
    layoutBinding.stageFlags = stageFlags;
    bindings[binding] = layoutBinding;
    return *this;
}

std::unique_ptr<MyDescriptorSetLayout> MyDescriptorSetLayout::Builder::build() const {
    return std::make_unique<MyDescriptorSetLayout>(myDevice, bindings);
}
 
// *************** Descriptor Set Layout *********************

MyDescriptorSetLayout::MyDescriptorSetLayout(
    Device &myDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings)
    : myDevice{myDevice}, bindings{bindings}{

    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
    for (auto kv : bindings) {
        setLayoutBindings.push_back(kv.second);
    }

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
    descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
    descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();

    if (vkCreateDescriptorSetLayout(myDevice.device(), &descriptorSetLayoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

MyDescriptorSetLayout::~MyDescriptorSetLayout() {
    vkDestroyDescriptorSetLayout(myDevice.device(), descriptorSetLayout, nullptr);
}
 
// *************** Descriptor Pool Builder *********************

MyDescriptorPool::Builder &MyDescriptorPool::Builder::addPoolSize(
    VkDescriptorType descriptorType, uint32_t count) {
    poolSizes.push_back({descriptorType, count});
    return *this;
}

MyDescriptorPool::Builder &MyDescriptorPool::Builder::setPoolFlags(
    VkDescriptorPoolCreateFlags flags) {
    poolFlags = flags;
    return *this;
}

MyDescriptorPool::Builder &MyDescriptorPool::Builder::setMaxSets(uint32_t count) {
    maxSets = count;
    return *this;
}
                 
std::unique_ptr<MyDescriptorPool> MyDescriptorPool::Builder::build() const {
    return std::make_unique<MyDescriptorPool>(myDevice, maxSets, poolFlags, poolSizes);
}

// *************** Descriptor Pool *********************

MyDescriptorPool::MyDescriptorPool(
    Device &lveDevice,
    uint32_t maxSets,
    VkDescriptorPoolCreateFlags poolFlags,
    const std::vector<VkDescriptorPoolSize> &poolSizes)
    : myDevice{lveDevice} {

    VkDescriptorPoolCreateInfo descriptorPoolInfo{};
    descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    descriptorPoolInfo.pPoolSizes = poolSizes.data();
    descriptorPoolInfo.maxSets = maxSets;
    descriptorPoolInfo.flags = poolFlags;

    if (vkCreateDescriptorPool(lveDevice.device(), &descriptorPoolInfo, nullptr, &descriptorPool) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

MyDescriptorPool::~MyDescriptorPool() {
    vkDestroyDescriptorPool(myDevice.device(), descriptorPool, nullptr);
}

bool MyDescriptorPool::allocateDescriptor(
    const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet &descriptor) const {
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.pSetLayouts = &descriptorSetLayout;
    allocInfo.descriptorSetCount = 1;

    // Might want to create a "DescriptorPoolManager" class that handles this case, and builds
    // a new pool whenever an old pool fills up. But this is beyond our current scope
    if (vkAllocateDescriptorSets(myDevice.device(), &allocInfo, &descriptor) != VK_SUCCESS) {
        return false;
    }
    return true;
}

void MyDescriptorPool::freeDescriptors(std::vector<VkDescriptorSet> &descriptors) const {
    vkFreeDescriptorSets(myDevice.device(), descriptorPool, static_cast<uint32_t>(descriptors.size()), descriptors.data());
}
 
void MyDescriptorPool::resetPool() {
    vkResetDescriptorPool(myDevice.device(), descriptorPool, 0);
}
 
// *************** Descriptor Writer *********************
 
MyDescriptorWriter::MyDescriptorWriter(MyDescriptorSetLayout &setLayout, MyDescriptorPool &pool)
    : setLayout{setLayout}, pool{pool} {}

MyDescriptorWriter &MyDescriptorWriter::writeBuffer(
    uint32_t binding, VkDescriptorBufferInfo *bufferInfo){
    assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

    auto &bindingDescription = setLayout.bindings[binding];

    assert(
        bindingDescription.descriptorCount == 1 &&
        "Binding single descriptor info, but binding expects multiple");

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorType = bindingDescription.descriptorType;
    write.dstBinding = binding;
    write.pBufferInfo = bufferInfo;
    write.descriptorCount = 1;

    writes.push_back(write);
    return *this;
}

MyDescriptorWriter &MyDescriptorWriter::writeImage(
    uint32_t binding, VkDescriptorImageInfo *imageInfo) {
    assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

    auto &bindingDescription = setLayout.bindings[binding];

    assert(
        bindingDescription.descriptorCount == 1 &&
        "Binding single descriptor info, but binding expects multiple");

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorType = bindingDescription.descriptorType;
    write.dstBinding = binding;
    write.pImageInfo = imageInfo;
    write.descriptorCount = 1;

    writes.push_back(write);
    return *this;
}
 
bool MyDescriptorWriter::build(VkDescriptorSet &set) {
    bool success = pool.allocateDescriptor(setLayout.getDescriptorSetLayout(), set);
    if (!success){
        return false;
    }
    overwrite(set);
    return true;
}
 
void MyDescriptorWriter::overwrite(VkDescriptorSet &set) {
    for (auto &write : writes){
        write.dstSet = set;
    }
    vkUpdateDescriptorSets(pool.myDevice.device(), writes.size(), writes.data(), 0, nullptr);
}
 
} 
 