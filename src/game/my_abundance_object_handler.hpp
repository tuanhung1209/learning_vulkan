#pragma once

#include "my_game_object.hpp"
#include "render_core/my_frame_info.hpp"

namespace my{

class BulletHandler{
public:
    BulletHandler(FrameInfo &frameInfo, MyModel &bulletModel);
    ~BulletHandler();

private:
    std::vector<BulletComponent> bullets;
    std::shared_ptr<MyModel> bulletModel;
};

}