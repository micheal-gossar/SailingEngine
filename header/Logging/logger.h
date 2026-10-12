#ifndef LOGGER_H
#define LOGGER_H

#include <fstream>
#include <functional>
#include <ostream>
#include <string_view>
#include <vector>

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define CYAN    "\033[36m"


struct StreamData {
    std::reference_wrapper<std::ostream> stream;
    const char* streamName;
    const char* color;
};

class Logger {
public:
    Logger();
    ~Logger() = default;

    void Log(std::string_view message, std::string_view streamName);
    void Flush();
    void addStream(std::reference_wrapper<std::ostream> stream, const char* streamName);
    void addStream(std::reference_wrapper<std::ostream> stream, const char* streamName, const char* color);

private:
    std::vector<StreamData> stream;
    std::ofstream logFile;
};

#endif
