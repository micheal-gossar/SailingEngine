#include "SailingEngine/Window/window.h"

#include <stdexcept>

Window::Window(Logger& logger)
    : windowLog{logger.CreateStream("Platform").Child("Window")} {
    windowLog.Debug("Initializing GLFW");
    if (glfwInit() != GLFW_TRUE) {
        windowLog.Error("GLFW initialization failed");
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, GAME_NAME, nullptr, nullptr);
    if (window == nullptr) {
        windowLog.Error("GLFW window creation failed");
        glfwTerminate();
        throw std::runtime_error("Failed to create the GLFW window");
    }
    windowLog.Info("Created the application window");
}

void Window::Show() {
    glfwShowWindow(window);
    windowLog.Debug("Application window is visible");
}

Window::~Window() {
    if (window != nullptr) {
        windowLog.Debug("Destroying the application window");
        glfwDestroyWindow(window);
    }
    glfwTerminate();
    windowLog.Info("GLFW shutdown complete");
}
