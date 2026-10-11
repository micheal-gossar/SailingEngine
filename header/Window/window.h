#ifndef WINDOW_H
#define WINDOW_H

#include "common.h"

class Window {
public:
    Window();
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    [[nodiscard]] GLFWwindow* GetWindow() const {
        return window;
    }

private:
    GLFWwindow* window = nullptr;
};
#endif
