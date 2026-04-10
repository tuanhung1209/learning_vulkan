#include "my_save_system.hpp"
#include "game/my_game_object.hpp"
#include "lib/json.hpp"
#include <fstream>
#include <memory>

using json = nlohmann::json;

namespace my {

SaveSystem::SaveSystem(Device &device) : myDevice{device} {};
SaveSystem::~SaveSystem() {};

void SaveSystem::saveScene(std::string saveFilePath, MyGameObject::Map &gameObjecs,
                           TerrainGenerator::TerrainConfig &config, SkyUbo &skyUbo,
                           GrassComputePush &grassPush) {
    // GameObjects
    json jsonGameObjectArray = json::array();
    for (auto &kv : gameObjecs) {
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
    json skyJson;
    skyJson["horizonColor"] = {skyUbo.horizonColor.x, skyUbo.horizonColor.y, skyUbo.horizonColor.z,
                               skyUbo.horizonColor.w};
    skyJson["skyColor"] = {skyUbo.skyColor.x, skyUbo.skyColor.y, skyUbo.skyColor.z, skyUbo.skyColor.w};
    skyJson["skyTextureColor"] = {skyUbo.skyTextureColor.x, skyUbo.skyTextureColor.y,
                                  skyUbo.skyTextureColor.z, skyUbo.skyTextureColor.w};
    skyJson["sunDirection"] = {skyUbo.sunDirection.x, skyUbo.sunDirection.y, skyUbo.sunDirection.z,
                               skyUbo.sunDirection.w};
    sceneJson["sky"] = skyJson;

    // Grass
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

    std::ofstream saveFile(saveFilePath);
    saveFile << sceneJson.dump(4);
}

void SaveSystem::loadScene(std::string loadFilePath, MyGameObject::Map &gameObjects,
                           TerrainGenerator::TerrainConfig &config, SkyUbo &skyUbo,
                           GrassComputePush &grassPush) {
    std::ifstream loadFile(loadFilePath);
    json jsonScene = json::parse(loadFile);

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

        gameObjects.emplace(obj.getId(), std::move(obj));
    }

    // Terrain
    auto &terrainData = jsonScene["terrain"];
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
    skyUbo.horizonColor = {skyData["horizonColor"][0], skyData["horizonColor"][1], skyData["horizonColor"][2],
                           skyData["horizonColor"][3]};
    skyUbo.skyColor = {skyData["skyColor"][0], skyData["skyColor"][1], skyData["skyColor"][2],
                       skyData["skyColor"][3]};
    skyUbo.skyTextureColor = {skyData["skyTextureColor"][0], skyData["skyTextureColor"][1],
                              skyData["skyTextureColor"][2], skyData["skyTextureColor"][3]};
    skyUbo.sunDirection = {skyData["sunDirection"][0], skyData["sunDirection"][1], skyData["sunDirection"][2],
                           skyData["sunDirection"][3]};

    // Grass
    auto &grassData = jsonScene["grass"];
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
}

}; // namespace my
