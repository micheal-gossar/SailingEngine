#include "SailingEngine/app.h"

#include "SailingEngine/Renderer/renderer.h"
#include "SailingEngine/Window/window.h"

#include <memory>

App::App(Logger& logger)
    : applicationLog{logger.CreateStream("Application")},
      window{std::make_unique<Window>(logger)},
      renderer{std::make_unique<Renderer>(logger, window.get())} {
    applicationLog.Info("Application subsystems initialized");
    window->Show();
}

App::~App() { applicationLog.Debug("Destroying application subsystems"); }

void App::Update() {
    applicationLog.Info("Entering the main loop");
    while (!glfwWindowShouldClose(window->GetWindow())) {
        glfwPollEvents();
        renderer->Update();
    }
    applicationLog.Debug("Window close requested; waiting for the renderer");
    renderer->WaitIdle();
    applicationLog.Info("Main loop stopped");
}
