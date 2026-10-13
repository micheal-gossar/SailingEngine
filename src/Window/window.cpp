#include "Window/window.h"

#include <stdexcept>

Window::Window() {
    if (glfwInit() != GLFW_TRUE) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, GAME_NAME, nullptr, nullptr);
    if (window == nullptr) {
        glfwTerminate();
        throw std::runtime_error("Failed to create the GLFW window");
    }
}

void Window::Show() {
    glfwShowWindow(window);
}

Window::~Window() {
    glfwDestroyWindow(window);
    glfwTerminate();
}
