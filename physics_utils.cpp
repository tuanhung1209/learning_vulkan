#include "physics_utils.hpp"

namespace my{
    OBB CollisionSystem::getOBB(MyGameObject &obj){
        OBB obb;

        glm::vec3 min = obj.model ? obj.model->getBound().min : glm::vec3(-0.5f);
        glm::vec3 max = obj.model ? obj.model->getBound().max : glm::vec3(0.5f);

        glm::vec3 scale = obj.transform.scale;
        glm::vec3 centerLocal = (min + max) * 0.5f;
        glm::vec3 extentsLocal = (max - min) * 0.5f;

        obb.extents = extentsLocal * glm::abs(scale);

        obb.axes = obj.transform.normalMatrix();

        obb.center = obj.transform.translation + (obb.axes * centerLocal);

        obb.axes[0] = glm::normalize(obb.axes[0]);
        obb.axes[1] = glm::normalize(obb.axes[1]);
        obb.axes[2] = glm::normalize(obb.axes[2]);

        return obb;
    };

    bool CollisionSystem::testAxis(const glm::vec3& axis, const OBB& obbA, const OBB& obbB,
                        float& minOverlap, glm::vec3& smallestAxis){

        float parallelAxis = glm::dot(axis, axis);
        if (parallelAxis < 1e-8f) return true;
        //TODO : invesigate this ???  
        glm::vec3 nAxis = axis / std::sqrt(parallelAxis);

        float rA = glm::abs(glm::dot(obbA.axes[0], nAxis) * obbA.extents.x) +
                   glm::abs(glm::dot(obbA.axes[1], nAxis) * obbA.extents.y) +
                   glm::abs(glm::dot(obbA.axes[2], nAxis) * obbA.extents.z);

        float rB = glm::abs(glm::dot(obbB.axes[0], nAxis) * obbB.extents.x) +
                   glm::abs(glm::dot(obbB.axes[1], nAxis) * obbB.extents.y) +
                   glm::abs(glm::dot(obbB.axes[2], nAxis) * obbB.extents.z);

        glm::vec3 distance = obbA.center - obbB.center;
        float t = glm::abs(glm::dot(distance, nAxis)); 

        //TODO : this part also need more understanding
        float overLap = (rA + rB) - t;
        if (overLap < 0.f) return false;

        if (overLap < minOverlap) {
            minOverlap = overLap;
            smallestAxis = nAxis;
            if (glm::dot(distance, smallestAxis) < 0.0f){
                smallestAxis = -smallestAxis;
            }
        }
        return true;
    }

    collisionManifold CollisionSystem::checkCollisionOBB(MyGameObject& objA, MyGameObject& objB){
        collisionManifold result{};
        result.depth = std::numeric_limits<float>::max();

        OBB obbA = getOBB(objA);
        OBB obbB = getOBB(objB);

        glm::vec3 axesToTest[15];
        int iaxes = 0;

        axesToTest[iaxes++] = obbA.axes[0];
        axesToTest[iaxes++] = obbA.axes[1];
        axesToTest[iaxes++] = obbA.axes[2];

        axesToTest[iaxes++] = obbB.axes[0];
        axesToTest[iaxes++] = obbB.axes[1];
        axesToTest[iaxes++] = obbB.axes[2];

        for (int i = 0; i < 3; i++){
            for (int j = 0; j < 3; j++){
                axesToTest[iaxes++] = glm::cross(obbA.axes[i], obbB.axes[j]);
            }
        }

        for (int i = 0; i < iaxes; i++){
            if (!testAxis(axesToTest[i], obbA, obbB, result.depth, result.normal)){
                return result; 
            }
        }
        result.isColliding = true;

        return result;        
    }

};