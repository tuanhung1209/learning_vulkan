#pragma once

#include "render_core/my_model.hpp"
#include "render_core/my_texture.hpp"
#include "vulkan_core/device.hpp"

#include <map>
#include <memory>
#include <string>

namespace my {
class AssetCache {
  public:
    AssetCache(Device &device);
    ~AssetCache();

    std::shared_ptr<MyModel> getModel(const std::string &path);
    std::shared_ptr<MyTexture> getTexture(const std::string &path);

    void clearModelCache();
    void clearTextureCache();

    // temp
    void insertModel(const std::string &path, std::shared_ptr<MyModel> model);

  private:
    Device &myDevice_;

    std::map<std::string, std::shared_ptr<MyModel>> modelCache{};
    std::map<std::string, std::shared_ptr<MyTexture>> textureCache{};
};
} // namespace my
