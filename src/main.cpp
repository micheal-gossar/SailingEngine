#include "SailingEngine/app.h"
#include "SailingEngine/Logging/logger.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

int main() {
    try {
        Logger logger;
        const LogStream mainLog = logger.CreateStream("Application").Child("Main");
        try {
            mainLog.Info("Starting Oblong Sailing Game");
            App app{logger};
            app.Update();
            mainLog.Info("Application exited successfully");
        } catch (const std::exception& exception) {
            mainLog.Error("Unhandled exception: " + std::string{exception.what()});
            logger.Flush();
            return EXIT_FAILURE;
        } catch (...) {
            mainLog.Error("Unhandled non-standard exception");
            logger.Flush();
            return EXIT_FAILURE;
        }
    } catch (const std::exception& e) {
        std::cerr << "Logger initialization failed: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
