#pragma once

#include <glm/glm.hpp>

namespace my {

struct Entity {
    uint32_t id;
    uint32_t generation;

    bool operator==(const Entity &otherEntity) const {
        return ((id == otherEntity.id) && (generation == otherEntity.generation));
    }

    bool operator!=(const Entity &otherEntity) const { return !(*this == otherEntity); }

    bool operator<(const Entity &otherEntity) const {
        if (id != otherEntity.id) return id < otherEntity.id;
        return generation < otherEntity.generation;
    }

    static const Entity null;

    bool isNull() const { return *this == null; }
};

inline const Entity Entity::null{~0u, 0};
} // namespace my
