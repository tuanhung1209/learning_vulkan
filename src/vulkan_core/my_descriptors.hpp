#pragma once 

#include "vulkan_core/device.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

namespace my{

class MyDescriptorSetLayout{
public:
    class Builder{
    public:
        Builder(Device &myDevice) : myDevice{myDevice} {}

        Builder &addBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlag, uint32_t count = 1);
        std::unique_ptr<MyDescriptorSetLayout> build() const;

    private:
        Device &myDevice;
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
    };

    MyDescriptorSetLayout(Device &myDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
    ~MyDescriptorSetLayout();

    MyDescriptorSetLayout(const MyDescriptorSetLayout &) = delete;
    MyDescriptorSetLayout &operator=(const MyDescriptorSetLayout &) = delete;

    VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

private:
    Device &myDevice;
    VkDescriptorSetLayout descriptorSetLayout;
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;

    friend class MyDescriptorWriter;
};

class MyDescriptorPool {
public :
    class Builder{
    public:
        Builder(Device &myDevice) : myDevice{myDevice} {}

        Builder &addPoolSize(VkDescriptorType descriptorType, uint32_t count);
        Builder &setPoolFlags(VkDescriptorPoolCreateFlags flags);
        Builder &setMaxSets(uint32_t count);

        std::unique_ptr<MyDescriptorPool> build() const;
    private:
        Device &myDevice;
        std::vector<VkDescriptorPoolSize> poolSizes{};
        uint32_t maxSets = 100;
        VkDescriptorPoolCreateFlags poolFlags = 0;
    };

MyDescriptorPool(Device &myDevice, uint32_t maxSets, VkDescriptorPoolCreateFlags poolFlags, const std::vector<VkDescriptorPoolSize> &poolSizes);
~MyDescriptorPool();

MyDescriptorPool(const MyDescriptorPool &) = delete;
MyDescriptorPool &operator=(const MyDescriptorPool &) = delete;

bool allocateDescriptor(const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet &descriptor) const;

void freeDescriptors(std::vector<VkDescriptorSet> &descriptors) const;

void resetPool();

private:
Device &myDevice;
VkDescriptorPool descriptorPool;

friend class MyDescriptorWriter;
};

class MyDescriptorWriter{
public:
MyDescriptorWriter(MyDescriptorSetLayout &setlayOut, MyDescriptorPool &pool);

MyDescriptorWriter &writeBuffer(uint32_t binding, VkDescriptorBufferInfo *bufferInfo);
MyDescriptorWriter &writeImage(uint32_t binding, VkDescriptorImageInfo *imageInfo);

bool build(VkDescriptorSet &set);
void overwrite(VkDescriptorSet &set);

private:
MyDescriptorSetLayout &setLayout;
MyDescriptorPool &pool;
std::vector<VkWriteDescriptorSet> writes;

};

}
