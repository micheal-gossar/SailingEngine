#ifndef WINDOW_H
#define WINDOW_H

#include "SailingEngine/Logging/logger.h"
#include "SailingEngine/common.h"

#include <GLFW/glfw3.h>

class Window {
public:
    explicit Window(Logger& logger);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    void Show();

    [[nodiscard]] GLFWwindow* GetWindow() const { return window; }

private:
    LogStream windowLog;
    GLFWwindow* window = nullptr;
};
#endif
