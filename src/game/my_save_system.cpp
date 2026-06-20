#include "my_save_system.hpp"
#include "game/my_game_object.hpp"
#include "lib/json.hpp"
#include <fstream>
#include <memory>

using json = nlohmann::json;

namespace my {

SaveSystem::SaveSystem(Device &device) : myDevice{device} {};
SaveSystem::~SaveSystem() {};

void SaveSystem::saveScene(const std::string &saveFilePath, SceneEntityRef scene) {
    json sceneJson;

    // Player
    if (scene.gameObjects.count(scene.playerId)) {
        auto &p = scene.gameObjects.at(scene.playerId);
        json playerJson;
        playerJson["translation"] = {p.transform.translation.x, p.transform.translation.y,
                                     p.transform.translation.z};
        playerJson["rotation"] = {p.transform.rotation.x, p.transform.rotation.y, p.transform.rotation.z};
        sceneJson["player"] = playerJson;
    }

    // GameObjects
    json jsonGameObjectArray = json::array();
    for (auto &kv : scene.gameObjects) {
        if (kv.first == scene.playerId) continue;
        auto &obj = kv.second;

        json jsonObj;
        jsonObj["id"] = kv.first;
        jsonObj["textureFilePath"] = obj.textureFilePath;
        jsonObj["modelFilePath"] = obj.modelFilePath;
        jsonObj["transform"]["translation"] = {obj.transform.translation.x, obj.transform.translation.y,
                                               obj.transform.translation.z};
        jsonObj["transform"]["rotation"] = {obj.transform.rotation.x, obj.transform.rotation.y,
                                            obj.transform.rotation.z};
        jsonObj["transform"]["scale"] = {obj.transform.scale.x, obj.transform.scale.y, obj.transform.scale.z};

        if (obj.pointLight) {
            jsonObj["pointLight"]["lightIntensity"] = obj.pointLight->lightIntensity;
            jsonObj["pointLight"]["color"] = {obj.color.x, obj.color.y, obj.color.z};
        }

        if (obj.rigidBody) {
            jsonObj["rigidBody"]["velocity"] = {obj.rigidBody->velocity.x, obj.rigidBody->velocity.y,
                                                obj.rigidBody->velocity.z};
            jsonObj["rigidBody"]["angularVelocity"] = {obj.rigidBody->angularVelocity.x,
                                                       obj.rigidBody->angularVelocity.y,
                                                       obj.rigidBody->angularVelocity.z};
            jsonObj["rigidBody"]["mass"] = obj.rigidBody->mass;
            jsonObj["rigidBody"]["restitution"] = obj.rigidBody->restitution;
            jsonObj["rigidBody"]["friction"] = obj.rigidBody->friction;
            jsonObj["rigidBody"]["linearDamping"] = obj.rigidBody->linearDamping;
            jsonObj["rigidBody"]["angularDamping"] = obj.rigidBody->angularDamping;
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
    skyJson["horizonColor"] = {skyPush.horizonColor.x, skyPush.horizonColor.y, skyPush.horizonColor.z,
                               skyPush.horizonColor.w};
    skyJson["skyColor"] = {skyPush.skyColor.x, skyPush.skyColor.y, skyPush.skyColor.z, skyPush.skyColor.w};
    skyJson["skyTextureColor"] = {skyPush.skyTextureColor.x, skyPush.skyTextureColor.y,
                                  skyPush.skyTextureColor.z, skyPush.skyTextureColor.w};
    skyJson["sunDirection"] = {skyPush.sunDirection.x, skyPush.sunDirection.y, skyPush.sunDirection.z,
                               skyPush.sunDirection.w};
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
    oceanJson["sunDirection"] = {ocean.sunDirection.x, ocean.sunDirection.y, ocean.sunDirection.z,
                                 ocean.sunDirection.w};
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

    // Player
    if (jsonScene.contains("player")) {
        auto &p = jsonScene["player"];
        auto &playerTransform = scene.gameObjects.at(scene.playerId).transform;
        playerTransform.translation = {p["translation"][0], p["translation"][1], p["translation"][2]};
        playerTransform.rotation = {p["rotation"][0], p["rotation"][1], p["rotation"][2]};
    }

    // GameObjects
    for (auto &oldObj : jsonScene["gameObjects"]) {
        auto obj = MyGameObject::createGameObjectWithId(oldObj["id"]);

        obj.modelFilePath = oldObj["modelFilePath"];
        obj.textureFilePath = oldObj["textureFilePath"];

        if (!obj.modelFilePath.empty()) {
            obj.model = MyModel::createModelFromFile(myDevice, obj.modelFilePath);
        }

        if (!obj.textureFilePath.empty()) {
            obj.texture = std::make_shared<MyTexture>(myDevice, obj.textureFilePath);
        }

        obj.transform.translation = {oldObj["transform"]["translation"][0],
                                     oldObj["transform"]["translation"][1],
                                     oldObj["transform"]["translation"][2]};
        obj.transform.rotation = {oldObj["transform"]["rotation"][0], oldObj["transform"]["rotation"][1],
                                  oldObj["transform"]["rotation"][2]};
        obj.transform.scale = {oldObj["transform"]["scale"][0], oldObj["transform"]["scale"][1],
                               oldObj["transform"]["scale"][2]};

        if (oldObj.contains("pointLight")) {
            obj.pointLight = std::make_unique<PointLightComponent>();
            obj.pointLight->lightIntensity = oldObj["pointLight"]["lightIntensity"];
            obj.color = {oldObj["pointLight"]["color"][0], oldObj["pointLight"]["color"][1],
                         oldObj["pointLight"]["color"][2]};
        }

        if (oldObj.contains("rigidBody")) {
            obj.rigidBody = std::make_unique<RigidBodyComponent>();
            obj.rigidBody->velocity = {oldObj["rigidBody"]["velocity"][0], oldObj["rigidBody"]["velocity"][1],
                                       oldObj["rigidBody"]["velocity"][2]};
            obj.rigidBody->angularVelocity = {oldObj["rigidBody"]["angularVelocity"][0],
                                              oldObj["rigidBody"]["angularVelocity"][1],
                                              oldObj["rigidBody"]["angularVelocity"][2]};
            obj.rigidBody->mass = oldObj["rigidBody"]["mass"];
            obj.rigidBody->restitution = oldObj["rigidBody"]["restitution"];
            obj.rigidBody->friction = oldObj["rigidBody"]["friction"];
            obj.rigidBody->linearDamping = oldObj["rigidBody"]["linearDamping"];
            obj.rigidBody->angularDamping = oldObj["rigidBody"]["angularDamping"];
        }

        scene.gameObjects.emplace(obj.getId(), std::move(obj));
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
    skyPush.horizonColor = {skyData["horizonColor"][0], skyData["horizonColor"][1],
                            skyData["horizonColor"][2], skyData["horizonColor"][3]};
    skyPush.skyColor = {skyData["skyColor"][0], skyData["skyColor"][1], skyData["skyColor"][2],
                        skyData["skyColor"][3]};
    skyPush.skyTextureColor = {skyData["skyTextureColor"][0], skyData["skyTextureColor"][1],
                               skyData["skyTextureColor"][2], skyData["skyTextureColor"][3]};
    skyPush.sunDirection = {skyData["sunDirection"][0], skyData["sunDirection"][1],
                            skyData["sunDirection"][2], skyData["sunDirection"][3]};

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
        ocean.sunDirection = {oceanData["sunDirection"][0], oceanData["sunDirection"][1],
                              oceanData["sunDirection"][2], oceanData["sunDirection"][3]};
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

}; // namespace my
