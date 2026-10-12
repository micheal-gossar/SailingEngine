#include "Logging/logger.h"

#include "common.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

Logger::Logger() {
    addStream(std::cout, "Log");

    const auto now = std::chrono::system_clock::now();
    const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    const std::tm* localTime = std::localtime(&nowTime);
    if (localTime == nullptr) {
        throw std::runtime_error("Failed to determine the local time for the log file");
    }

    std::ostringstream fileName;
    fileName << "log_"
             << std::put_time(localTime, "%Y-%m-%d_%H-%M-%S")
             << ".txt";

    const std::filesystem::path logDirectory{"logFile"};
    std::filesystem::create_directories(logDirectory);
    logFile.open(logDirectory / fileName.str());
    if (!logFile.is_open()) {
        throw std::runtime_error("Failed to open the log file");
    }

    addStream(logFile, "Log");
    addStream(std::cerr, "Error", RED);
    if (inDebug) {
        addStream(std::cout, "Debug", BLUE);
        addStream(logFile, "Debug", BLUE);
    }
}

void Logger::Log(std::string_view message, std::string_view streamName)
{
    for (const StreamData& streamData : stream) {
        if (std::string_view{streamData.streamName} == streamName) {
            streamData.stream.get()
                << streamData.color
                << streamName << " : "
                << message
                << RESET
                << '\n';
        }
    }
}

void Logger::Flush()
{
    for (const StreamData& streamData : stream) {
        streamData.stream.get().flush();
    }
}

void Logger::addStream(std::reference_wrapper<std::ostream> s, const char *streamName)
{
    stream.push_back({s, streamName, RESET});
}

void Logger::addStream(
    std::reference_wrapper<std::ostream> s,
    const char* streamName,
    const char* color)
{
    stream.push_back({s, streamName, color});
}