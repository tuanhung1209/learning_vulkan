#include "game/first_app.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace my {
void runEcsTest();
} // namespace my

int main(int argc, char *argv[]) {
    Mode mode = Mode::Wallpaper;

    for (int i = 0; i < argc; i++) {
        const std::string arg = argv[i];
        if (arg == "--edit") { mode = Mode::Edit; }
        if (arg == "--test-ecs") {
            my::runEcsTest();
            return EXIT_SUCCESS;
        }
    }

    my::FirstApp app{mode};

    try {
        app.run();
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
