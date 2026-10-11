#ifndef APP_H
#define APP_H

#include "common.h"
#include "Renderer/renderer.h"
#include "Window/window.h"

class App {
public:
    App();
    ~App();

    Window window;
    Renderer renderer;

    void Update();
};

#endif
