#pragma once

#include "entity.hpp"
#include "component_pool.hpp"
#include "icomponent_pool.hpp"

#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace my {

template <typename... T> class QueryEntities;

class EcsManager {
  public:
    EcsManager() = default;
    ~EcsManager() = default;

    EcsManager(const EcsManager &) = delete;
    EcsManager &operator=(const EcsManager &) = delete;
    EcsManager(EcsManager &&) = default;
    EcsManager &operator=(EcsManager &&) = default;

    Entity createEntity() {
        uint32_t id;
        uint32_t generation;
        if (freeSlots.size() != 0) {
            id = freeSlots.back();
            freeSlots.pop_back();
            generation = generations[id];
        } else {
            id = generations.size();
            generations.push_back(0);
            generation = 0;
        }
        return Entity{id, generation};
    }

    void destroyEntity(Entity entity) {
        if (!isEntityAlive(entity)) return;
        for (auto &[type, pool] : pools) { pool->removeComponent(entity); }
        generations[entity.id]++;
        freeSlots.push_back(entity.id);
    }

    bool isEntityAlive(Entity entity) const {
        if (entity.id >= generations.size()) return false;
        return generations[entity.id] == entity.generation;
    }

    void clearEcs() {
        for (auto &[type, pool] : pools) { pool->clearComponents(); }
        generations.clear();
        freeSlots.clear();
    }

    template <typename T> void add(Entity entity, T component) {
        auto *pool = getOrCreatePool<T>();
        pool->insertComponent(entity, std::move(component));
    }

    template <typename T> T *get(Entity entity) {
        auto *pool = findPool<T>();
        return pool ? pool->getComponent(entity) : nullptr;
    }

    template <typename T> bool has(Entity entity) {
        auto *pool = findPool<T>();
        return pool && pool->containComponent(entity);
    }

    template <typename T> void remove(Entity entity) {
        auto *pool = findPool<T>();
        if (pool) pool->removeComponent(entity);
    }

    template <typename T> ComponentPool<T> *pool() { return findPool<T>(); }

    template <typename... T> QueryEntities<T...> query() { return QueryEntities<T...>{*this}; }

  private:
    template <typename T> ComponentPool<T> *getOrCreatePool() {
        std::type_index key = std::type_index(typeid(T));
        auto it = pools.find(key);
        if (it == pools.end()) {
            auto poolPtr = std::make_unique<ComponentPool<T>>();
            ComponentPool<T> *rawPtr = poolPtr.get();
            pools.emplace(key, std::move(poolPtr));
            return rawPtr;
        }
        return static_cast<ComponentPool<T> *>(it->second.get());
    }

    template <typename T> ComponentPool<T> *findPool() {
        std::type_index key = std::type_index(typeid(T));
        auto it = pools.find(key);
        if (it == pools.end()) return nullptr;
        return static_cast<ComponentPool<T> *>(it->second.get());
    }

    std::vector<uint32_t> generations{};
    std::vector<uint32_t> freeSlots{};
    std::unordered_map<std::type_index, std::unique_ptr<IComponentPool>> pools;
};
} // namespace my

// breaking template circular deps.
#include "ecs/query_entity.hpp"
