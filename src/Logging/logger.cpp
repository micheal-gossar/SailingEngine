#include "Logging/logger.h"

#include "common.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
constexpr std::string_view logFilePrefix{"log_"};
constexpr std::string_view logFileSuffix{".txt"};
constexpr std::size_t logTimestampLength = 19;
constexpr auto logRetention = std::chrono::days{30};

std::optional<std::time_t> getLogTimestamp(
    const std::filesystem::path& path)
{
    const std::string fileName = path.filename().string();
    if (!fileName.starts_with(logFilePrefix) ||
        !fileName.ends_with(logFileSuffix) ||
        fileName.size() !=
            logFilePrefix.size() +
                logTimestampLength +
                logFileSuffix.size()) {
        return std::nullopt;
    }

    const std::string timestampText = fileName.substr(
        logFilePrefix.size(),
        fileName.size() - logFilePrefix.size() - logFileSuffix.size());

    std::tm localTime{};
    localTime.tm_isdst = -1;
    std::istringstream timestampStream{timestampText};
    timestampStream >> std::get_time(
        &localTime,
        "%Y-%m-%d_%H-%M-%S");
    if (timestampStream.fail()) {
        return std::nullopt;
    }

    const std::time_t timestamp = std::mktime(&localTime);
    if (timestamp == static_cast<std::time_t>(-1)) {
        return std::nullopt;
    }

    std::ostringstream normalizedTimestamp;
    normalizedTimestamp << std::put_time(
        &localTime,
        "%Y-%m-%d_%H-%M-%S");
    if (normalizedTimestamp.str() != timestampText) {
        return std::nullopt;
    }

    return timestamp;
}

void deleteExpiredLogFiles(
    const std::filesystem::path& logDirectory,
    std::chrono::system_clock::time_point now)
{
    const std::time_t expirationTime =
        std::chrono::system_clock::to_time_t(now - logRetention);
    for (const auto& entry :
         std::filesystem::directory_iterator(logDirectory)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        const auto timestamp = getLogTimestamp(entry.path());
        if (timestamp.has_value() && *timestamp < expirationTime) {
            std::filesystem::remove(entry.path());
        }
    }
}
}

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
    deleteExpiredLogFiles(logDirectory, now);
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