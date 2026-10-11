#ifndef RENDERER_H
#define RENDERER_H

#include "common.h"

class Renderer {
public:
    Renderer();

    void InitVulkan();

private:
    void createInstance();

    std::vector<const char*> getRequiredInstanceExtensions();
    vk::raii::Context context;
    vk::raii::Instance instance = nullptr;
};

#endif
