#include "physics_utils.hpp"

namespace my{
    OBB getOBB(MyGameObject &obj){
        OBB obb;

        glm::vec3 min = obj.model ? obj.model->getBound().min : glm::vec3(-0.5f);
        glm::vec3 max = obj.model ? obj.model->getBound().max : glm::vec3(0.5f);

        glm::vec3 scale = obj.transform.scale;
        glm::vec3 centerLocal = (min + max) * 0.5f;
        glm::vec3 extentsLocal = (min - max) * 0.5f;

        obb.extents = extentsLocal * glm::abs(scale);

        obb.axes = obj.transform.normalMatrix();

        obb.center = obj.transform.translation + (obb.axes * centerLocal);

        obb.axes[0] = glm::normalize(obb.axes[0]);
        obb.axes[1] = glm::normalize(obb.axes[1]);
        obb.axes[2] = glm::normalize(obb.axes[2]);

        return obb;
    };
};