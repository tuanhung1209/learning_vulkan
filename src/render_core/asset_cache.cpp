#include "render_core/asset_cache.hpp"
#include <memory>

namespace my {

AssetCache::AssetCache(Device &device) : myDevice_(device) {};
AssetCache::~AssetCache() = default;

std::shared_ptr<MyModel> AssetCache::getModel(const std::string &path) {
    if (modelCache.find(path) == modelCache.end()) {
        std::shared_ptr<MyModel> model = MyModel::createModelFromFile(myDevice_, path);
        modelCache[path] = model;
        return model;
    }
    return modelCache[path];
}

std::shared_ptr<MyTexture> AssetCache::getTexture(const std::string &path) {
    if (textureCache.find(path) == textureCache.end()) {
        std::shared_ptr<MyTexture> texture = std::make_shared<MyTexture>(myDevice_, path);
        textureCache[path] = texture;
        return texture;
    }
    return textureCache[path];
}

void AssetCache::insertModel(const std::string &filepath, std::shared_ptr<MyModel> model) {
    modelCache[filepath] = model;
}

void AssetCache::clearModelCache() { modelCache.clear(); }
void AssetCache::clearTextureCache() { textureCache.clear(); }

} // namespace my
