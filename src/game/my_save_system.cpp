#include "my_save_system.hpp"

#include "ecs/components/transform_component.hpp"
#include "ecs/components/model_component.hpp"
#include "ecs/components/texture_component.hpp"
#include "ecs/components/point_light_component.hpp"
#include "game_components/bullet_component.hpp"

#include "lib/json.hpp"

#include <fstream>

namespace my {

using json = nlohmann::json;

SaveSystem::SaveSystem(Device &device) : myDevice{device} {};
SaveSystem::~SaveSystem() {};

void SaveSystem::saveScene(const std::string &saveFilePath, SceneEntityRef scene) {
    json sceneJson;

    // Player
    if (scene.ecsManager.isEntityAlive(scene.playerEntity)) {
        auto *t = scene.ecsManager.get<TransformComponent>(scene.playerEntity);
        if (t) {
            json playerJson;
            playerJson["translation"] = {t->translation.x, t->translation.y, t->translation.z};
            playerJson["rotation"] = {t->rotation.x, t->rotation.y, t->rotation.z};
            sceneJson["player"] = playerJson;
        }
    }

    // Drawable entities (have Transform + Model + Texture).
    // Bullets are transient — skip them. The player is saved separately above.
    json jsonGameObjectArray = json::array();
    for (auto [e, t, m, tex] :
         scene.ecsManager.query<TransformComponent, ModelComponent, TextureComponent>()) {

        if (e == scene.playerEntity) continue;
        if (scene.ecsManager.has<BulletComponent>(e)) continue;

        json jsonObj;
        jsonObj["id"] = e.id; // hint only — loader creates fresh entities
        jsonObj["modelFilePath"] = m.modelPath;
        jsonObj["textureFilePath"] = tex.texturePath;
        jsonObj["transform"]["translation"] = {t.translation.x, t.translation.y, t.translation.z};
        jsonObj["transform"]["rotation"] = {t.rotation.x, t.rotation.y, t.rotation.z};
        jsonObj["transform"]["scale"] = {t.scale.x, t.scale.y, t.scale.z};

        if (scene.ecsManager.has<PointLightComponent>(e)) {
            auto *pl = scene.ecsManager.get<PointLightComponent>(e);
            auto *c = scene.ecsManager.get<ColorComponent>(e);
            if (pl) {
                jsonObj["pointLight"]["lightIntensity"] = pl->lightIntensity;
                if (c) jsonObj["pointLight"]["color"] = {c->rgb.x, c->rgb.y, c->rgb.z};
            }
        }

        jsonGameObjectArray.push_back(jsonObj);
    }
    sceneJson["gameObjects"] = jsonGameObjectArray;

    // Terrain
    auto &config = scene.terrainConfig;
    json terrainJson;
    terrainJson["seed"] = config.seed;
    terrainJson["noiseScale"] = config.noiseScale;
    terrainJson["octaves"] = config.octaves;
    terrainJson["heightScale"] = config.heightScale;
    terrainJson["resolution"] = config.resolution;
    terrainJson["rotationAngle"] = config.rotationAngle;
    terrainJson["lacunarity"] = config.lacunarity;
    terrainJson["persistence"] = config.persistence;
    terrainJson["sandThreshold"] = config.sandThreshold;
    terrainJson["grassThreshold"] = config.grassThreshold;
    terrainJson["rockThreshold"] = config.rockThreshold;
    sceneJson["terrain"] = terrainJson;

    // Sky
    auto &skyPush = scene.skyConfig;
    json skyJson;
    skyJson["skyColor"] = {skyPush.skyColor.x, skyPush.skyColor.y, skyPush.skyColor.z, skyPush.skyColor.w};
    skyJson["skyTextureColor"] = {skyPush.skyTextureColor.x, skyPush.skyTextureColor.y,
                                  skyPush.skyTextureColor.z, skyPush.skyTextureColor.w};
    sceneJson["sky"] = skyJson;

    // Grass
    auto &grassPush = scene.grassConfig;
    json grassJson;
    grassJson["gridSize"] = grassPush.gridSize;
    grassJson["terrainResolution"] = grassPush.terrainResolution;
    grassJson["heightScale"] = grassPush.heightScale;
    grassJson["spacing"] = grassPush.spacing;
    grassJson["bladeHeight"] = grassPush.bladeHeight;
    grassJson["windDirX"] = grassPush.windDirX;
    grassJson["windDirZ"] = grassPush.windDirZ;
    grassJson["windFreq"] = grassPush.windFreq;
    grassJson["windAmplitude"] = grassPush.windAmplitude;
    grassJson["turbPower"] = grassPush.turbPower;
    grassJson["turbSize"] = grassPush.turbSize;
    grassJson["droopStrength"] = grassPush.droopStrength;
    grassJson["xPeriod"] = grassPush.xPeriod;
    grassJson["yPeriod"] = grassPush.yPeriod;
    grassJson["windBias"] = grassPush.windBias;
    grassJson["baseColor"] = {grassPush.baseColor.x, grassPush.baseColor.y, grassPush.baseColor.z,
                              grassPush.baseColor.w};
    grassJson["tipColor"] = {grassPush.tipColor.x, grassPush.tipColor.y, grassPush.tipColor.z,
                             grassPush.tipColor.w};
    sceneJson["grass"] = grassJson;

    // Ocean
    auto &ocean = scene.oceanConfig;
    json oceanJson;
    oceanJson["horizonColor"] = {ocean.horizonColor.x, ocean.horizonColor.y, ocean.horizonColor.z,
                                 ocean.horizonColor.w};
    oceanJson["skyColor"] = {ocean.skyColor.x, ocean.skyColor.y, ocean.skyColor.z, ocean.skyColor.w};
    oceanJson["deepColor"] = {ocean.deepColor.x, ocean.deepColor.y, ocean.deepColor.z, ocean.deepColor.w};
    json wavesJson = json::array();
    for (int i = 0; i < MAX_OCEAN_WAVES; i++) {
        json w;
        w["direction"] = {ocean.waves[i].direction.x, ocean.waves[i].direction.y};
        w["frequency"] = ocean.waves[i].frequency;
        w["amplitude"] = ocean.waves[i].amplitude;
        w["steepness"] = ocean.waves[i].steepness;
        w["speed"] = ocean.waves[i].speed;
        wavesJson.push_back(w);
    }
    oceanJson["waves"] = wavesJson;
    sceneJson["ocean"] = oceanJson;

    std::ofstream saveFile(saveFilePath);
    saveFile << sceneJson.dump(4);
}

void SaveSystem::loadScene(const std::string &loadFilePath, SceneEntityRef scene) {
    std::ifstream loadFile(loadFilePath);
    json jsonScene = json::parse(loadFile);

    // Player — assumed already created by caller (first_app does createEntity()
    // before loadScene). We just write the saved transform into it.
    if (jsonScene.contains("player")) {
        auto &p = jsonScene["player"];
        auto *t = scene.ecsManager.get<TransformComponent>(scene.playerEntity);
        if (t) {
            t->translation = {p["translation"][0], p["translation"][1], p["translation"][2]};
            t->rotation = {p["rotation"][0], p["rotation"][1], p["rotation"][2]};
        }
    }

    // Drawable entities. Each loaded entry becomes a fresh entity in the registry.
    // The saved "id" is a hint only — generations are runtime state and don't
    // persist across runs, so we never trust an old (id, gen) pair.
    for (auto &oldObj : jsonScene["gameObjects"]) {
        Entity obj = scene.ecsManager.createEntity();

        TransformComponent t;
        t.translation = {oldObj["transform"]["translation"][0], oldObj["transform"]["translation"][1],
                         oldObj["transform"]["translation"][2]};
        t.rotation = {oldObj["transform"]["rotation"][0], oldObj["transform"]["rotation"][1],
                       oldObj["transform"]["rotation"][2]};
        t.scale = {oldObj["transform"]["scale"][0], oldObj["transform"]["scale"][1],
                    oldObj["transform"]["scale"][2]};
        scene.ecsManager.add<TransformComponent>(obj, t);

        std::string modelPath = oldObj["modelFilePath"];
        std::string texPath = oldObj["textureFilePath"];
        if (!modelPath.empty()) scene.ecsManager.add<ModelComponent>(obj, ModelComponent{modelPath});
        if (!texPath.empty()) scene.ecsManager.add<TextureComponent>(obj, TextureComponent{texPath});

        if (oldObj.contains("pointLight")) {
            scene.ecsManager.add<PointLightComponent>(
                obj, PointLightComponent{oldObj["pointLight"]["lightIntensity"].get<float>()});
            if (oldObj["pointLight"].contains("color")) {
                scene.ecsManager.add<ColorComponent>(
                    obj, ColorComponent{{oldObj["pointLight"]["color"][0], oldObj["pointLight"]["color"][1],
                                          oldObj["pointLight"]["color"][2]}});
            }
        }
    }

    // Terrain
    auto &terrainData = jsonScene["terrain"];
    auto &config = scene.terrainConfig;
    config.seed = terrainData["seed"];
    config.noiseScale = terrainData["noiseScale"];
    config.octaves = terrainData["octaves"];
    config.heightScale = terrainData["heightScale"];
    config.resolution = terrainData["resolution"];
    config.rotationAngle = terrainData["rotationAngle"];
    config.lacunarity = terrainData["lacunarity"];
    config.persistence = terrainData["persistence"];
    config.sandThreshold = terrainData["sandThreshold"];
    config.grassThreshold = terrainData["grassThreshold"];
    config.rockThreshold = terrainData["rockThreshold"];

    // Sky
    auto &skyData = jsonScene["sky"];
    auto &skyPush = scene.skyConfig;
    skyPush.skyColor = {skyData["skyColor"][0], skyData["skyColor"][1], skyData["skyColor"][2],
                       skyData["skyColor"][3]};
    skyPush.skyTextureColor = {skyData["skyTextureColor"][0], skyData["skyTextureColor"][1],
                               skyData["skyTextureColor"][2], skyData["skyTextureColor"][3]};

    // Grass
    auto &grassData = jsonScene["grass"];
    auto &grassPush = scene.grassConfig;
    grassPush.gridSize = grassData["gridSize"];
    grassPush.terrainResolution = grassData["terrainResolution"];
    grassPush.heightScale = grassData["heightScale"];
    grassPush.spacing = grassData["spacing"];
    grassPush.bladeHeight = grassData["bladeHeight"];
    grassPush.windDirX = grassData["windDirX"];
    grassPush.windDirZ = grassData["windDirZ"];
    grassPush.windFreq = grassData["windFreq"];
    grassPush.windAmplitude = grassData["windAmplitude"];
    grassPush.turbPower = grassData["turbPower"];
    grassPush.turbSize = grassData["turbSize"];
    grassPush.droopStrength = grassData["droopStrength"];
    grassPush.xPeriod = grassData["xPeriod"];
    grassPush.yPeriod = grassData["yPeriod"];
    grassPush.windBias = grassData["windBias"];
    grassPush.baseColor = {grassData["baseColor"][0], grassData["baseColor"][1], grassData["baseColor"][2],
                           grassData["baseColor"][3]};
    grassPush.tipColor = {grassData["tipColor"][0], grassData["tipColor"][1], grassData["tipColor"][2],
                          grassData["tipColor"][3]};

    // Ocean
    if (jsonScene.contains("ocean")) {
        auto &oceanData = jsonScene["ocean"];
        auto &ocean = scene.oceanConfig;
        ocean.horizonColor = {oceanData["horizonColor"][0], oceanData["horizonColor"][1],
                              oceanData["horizonColor"][2], oceanData["horizonColor"][3]};
        ocean.skyColor = {oceanData["skyColor"][0], oceanData["skyColor"][1], oceanData["skyColor"][2],
                          oceanData["skyColor"][3]};
        ocean.deepColor = {oceanData["deepColor"][0], oceanData["deepColor"][1], oceanData["deepColor"][2],
                           oceanData["deepColor"][3]};
        for (int i = 0; i < MAX_OCEAN_WAVES; i++) {
            ocean.waves[i].direction = {oceanData["waves"][i]["direction"][0],
                                        oceanData["waves"][i]["direction"][1]};
            ocean.waves[i].frequency = oceanData["waves"][i]["frequency"];
            ocean.waves[i].amplitude = oceanData["waves"][i]["amplitude"];
            ocean.waves[i].steepness = oceanData["waves"][i]["steepness"];
            ocean.waves[i].speed = oceanData["waves"][i]["speed"];
        }
    }
}

} // namespace my