#pragma once

#include "ecs/ecs_manager.hpp"
#include "ecs/component_pool.hpp"
#include "ecs/icomponent_pool.hpp"

#include <tuple>

namespace my {

template <typename... T> class IteratorComponentPool {
  public:
    IteratorComponentPool<T...>(IComponentPool *smallestPool, std::tuple<ComponentPool<T> *...> *pools,
                                uint32_t slot)
        : smallestPool_{smallestPool}, pools_{pools}, slot_{slot} {
        skipToMatch();
    }
    ~IteratorComponentPool<T...>() = default;

    bool operator!=(const IteratorComponentPool &other) { return slot_ != other.slot_; }

    IteratorComponentPool &operator++() {
        ++slot_;
        skipToMatch();
        return *this;
    }

    std::tuple<Entity, T &...> operator*() {
        Entity entity = smallestPool_->getEntityAt(slot_);
        return std::tuple<Entity, T &...>(entity,
                                          *std::get<ComponentPool<T> *>(*pools_)->getComponent(entity)...);
    }

  private:
    bool poolMatch(Entity entity) {
        return (std::get<ComponentPool<T> *>(*pools_)->containComponent(entity) && ...);
    };

    void skipToMatch() {
        while (slot_ < smallestPool_->getComponentSize() && !poolMatch(smallestPool_->getEntityAt(slot_))) {
            slot_++;
        }
    };

    IComponentPool *smallestPool_;
    std::tuple<ComponentPool<T> *...> *pools_;
    uint32_t slot_;
};

template <typename... T> class QueryEntities {
  public:
    QueryEntities(EcsManager &manager) : pools{manager.pool<T>()...} {
        smallestPool = std::get<0>(pools);
        uint32_t min_pool_size = smallestPool->getComponentSize();

        bool anyNull = (... || (std::get<ComponentPool<T> *>(pools) == nullptr));
        if (anyNull) {
            smallestPool = nullptr;
            return;
        }

        (
            [&] {
                auto *p = std::get<ComponentPool<T> *>(pools);
                if (p->getComponentSize() < min_pool_size) {
                    smallestPool = p;
                    min_pool_size = p->getComponentSize();
                }
            }(),
            ...);
    };

    ~QueryEntities() = default;

    // TODO: add a guard when not using a populated component
    IteratorComponentPool<T...> begin() { return IteratorComponentPool<T...>(smallestPool, &pools, 0); };
    IteratorComponentPool<T...> end() {
        return IteratorComponentPool<T...>(smallestPool, &pools, smallestPool->getComponentSize());
    };

  private:
    std::tuple<ComponentPool<T> *...> pools;
    IComponentPool *smallestPool;
};

} // namespace my
