#include "app.h"

#include <cstdlib>
#include <exception>
#include <iostream>

int main() {
    try {
        App app;
        app.Update();
    }
    catch(const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
