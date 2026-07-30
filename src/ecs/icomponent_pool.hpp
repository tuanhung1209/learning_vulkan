#pragma once

#include "entity.hpp"

namespace my {

class IComponentPool {
  public:
    IComponentPool() = default;
    virtual ~IComponentPool() = default;

    virtual bool containComponent(Entity entity) = 0;
    virtual void removeComponent(Entity entity) = 0;
    virtual uint32_t getComponentSize() = 0;
    virtual void clearComponents() = 0;
    virtual Entity getEntityAt(uint32_t slot) = 0;

  private:
};

} // namespace my
