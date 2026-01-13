#pragma once

#include "my_game_object.hpp"
#include "render_core/my_frame_info.hpp"

namespace my{

class BulletHandler{
public:
    BulletHandler(std::shared_ptr<MyModel> model);
    ~BulletHandler();

    void spawnBullet(glm::vec3 position, glm::vec3 velocity, glm::vec3 rotation);

    void update(float dt);

    void renderBullet(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);
private:
    std::vector<std::unique_ptr<MyGameObject>> bullets;
    std::shared_ptr<MyModel> bulletModel;
};

}