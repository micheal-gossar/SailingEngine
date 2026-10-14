#ifndef APP_H
#define APP_H

#include "SailingEngine/Logging/logger.h"

#include <memory>

class Renderer;
class Window;

class App {
public:
    explicit App(Logger& logger);
    ~App();

    void Update();

private:
    LogStream applicationLog;
    std::unique_ptr<Window> window;
    std::unique_ptr<Renderer> renderer;
};

#endif
