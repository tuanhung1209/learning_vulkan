#pragma once

#include "icomponent_pool.hpp"
#include "entity.hpp"

#include <cstdint>
#include <vector>

namespace my {

template <typename T> class ComponentPool : public IComponentPool {
  public:
    bool containComponent(Entity entity) override {
        if (entity.id >= sparseSet.size()) return false;
        return sparseSet[entity.id] != NULL_SLOT;
    }

    void insertComponent(Entity entity, T component) {
        ensureSparseSize(entity.id);
        uint32_t slot = sparseSet[entity.id];
        if (slot == NULL_SLOT) {
            sparseSet[entity.id] = packedComponent.size();
            packedEntities.push_back(entity);
            packedComponent.push_back(std::move(component));
        } else {
            packedComponent[slot] = std::move(component);
        }
    }

    void removeComponent(Entity entity) override {
        if (!containComponent(entity)) return;
        uint32_t slot = sparseSet[entity.id];
        uint32_t lastComponent = packedComponent.size() - 1;

        if (slot != lastComponent) {
            Entity lastEntity = packedEntities[lastComponent];
            packedComponent[slot] = std::move(packedComponent.back());
            packedEntities[slot] = lastEntity;
            sparseSet[lastEntity.id] = slot;
        }

        packedComponent.pop_back();
        packedEntities.pop_back();
        sparseSet[entity.id] = NULL_SLOT;
    }

    T *getComponent(Entity entity) {
        if (entity.id >= sparseSet.size()) return nullptr;
        uint32_t slot = sparseSet[entity.id];
        return slot == NULL_SLOT ? nullptr : &packedComponent[slot];
    }

    void clearComponents() override {
        for (auto e : packedEntities) sparseSet[e.id] = NULL_SLOT;
        packedEntities.clear();
        packedComponent.clear();
    }

    uint32_t getComponentSize() override { return packedEntities.size(); }

    Entity getEntityAt(uint32_t slot) override { return packedEntities[slot]; }

  private:
    std::vector<uint32_t> sparseSet{};
    std::vector<T> packedComponent{};
    std::vector<Entity> packedEntities{};

    void ensureSparseSize(uint32_t id) {
        if (id >= sparseSet.size()) sparseSet.resize(id + 1, NULL_SLOT);
    }

    static constexpr uint32_t NULL_SLOT = ~0u;
};
} // namespace my
