#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/common.hpp>
#include <glm/glm.hpp>

namespace my {

struct PointLightComponent {
    float lightIntensity = 1.0f;
}; // what about the point light push constand, should i change to take the transform too or somthing like
// that ?

} // namespace my
