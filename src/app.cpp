#include "app.h"

App::App() {
    
}

App::~App() = default;

void App::Update() {
    while (!glfwWindowShouldClose(window.GetWindow())) {
        glfwPollEvents();
    }
}
