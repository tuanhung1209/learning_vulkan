#include "game/first_app.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

int main(int argc, char *argv[]) {
    Mode mode = Mode::Wallpaper;

    for (int i = 0; i < argc; i++) {
        if (std::string(argv[i]) == "--edit") { mode = Mode::Edit; }
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
