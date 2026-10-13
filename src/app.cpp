#include "app.h"

#include "Renderer/renderer.h"
#include "Window/window.h"

#include <memory>

App::App()
    : window{std::make_unique<Window>()},
      renderer{std::make_unique<Renderer>(window.get())}
{
    window->Show();
}

App::~App() = default;

void App::Update() {
    while (!glfwWindowShouldClose(window->GetWindow())) {
        glfwPollEvents();
    }
}
