#ifndef WINDOW_H
#define WINDOW_H

#include "common.h"

#include <GLFW/glfw3.h>

class Window {
public:
    Window();
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    void Show();

    [[nodiscard]] GLFWwindow* GetWindow() const {
        return window;
    }

private:
    GLFWwindow* window = nullptr;

};
#endif
