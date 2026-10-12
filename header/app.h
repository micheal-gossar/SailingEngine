#ifndef APP_H
#define APP_H

#include <memory>

class Renderer;
class Window;

class App {
public:
    App();
    ~App();

    void Update();

private:
    std::unique_ptr<Window> window;
    std::unique_ptr<Renderer> renderer;
};

#endif
