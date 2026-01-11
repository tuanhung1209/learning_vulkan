#include "game/first_app.hpp"

#include <iostream>
#include <stdexcept>
#include <cstdlib>

int main(){

    my::FirstApp app{};

    try
    {
        app.run();
    }
    catch (const std::exception &e){
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
