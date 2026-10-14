#include "SailingEngine/Logging/logger.h"

#include "SailingEngine/common.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {
constexpr std::string_view logFilePrefix{"log_"};
constexpr std::string_view logFileSuffix{".txt"};
constexpr std::size_t logTimestampLength = 19;
constexpr auto logRetention = std::chrono::days{30};
constexpr std::string_view colorReset{"\033[0m"};
constexpr std::string_view colorRed{"\033[31m"};
constexpr std::string_view colorYellow{"\033[33m"};
constexpr std::string_view colorBlue{"\033[34m"};
constexpr std::size_t categoryColumnWidth = 52;

std::tm getLocalTime(std::time_t timestamp) {
    std::tm localTime{};
#ifdef _WIN32
    if (localtime_s(&localTime, &timestamp) != 0) {
        throw std::runtime_error("Failed to determine the local time");
    }
#else
    if (localtime_r(&timestamp, &localTime) == nullptr) {
        throw std::runtime_error("Failed to determine the local time");
    }
#endif
    return localTime;
}

std::string formatLogTimestamp(std::chrono::system_clock::time_point time) {
    const std::time_t timestamp = std::chrono::system_clock::to_time_t(time);
    const std::tm localTime = getLocalTime(timestamp);
    std::ostringstream timestampStream;
    timestampStream << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return timestampStream.str();
}

std::optional<std::time_t> getLogTimestamp(const std::filesystem::path& path) {
    const std::string fileName = path.filename().string();
    if (!fileName.starts_with(logFilePrefix) || !fileName.ends_with(logFileSuffix) ||
        fileName.size() != logFilePrefix.size() + logTimestampLength + logFileSuffix.size()) {
        return std::nullopt;
    }

    const std::string timestampText = fileName.substr(
        logFilePrefix.size(), fileName.size() - logFilePrefix.size() - logFileSuffix.size());

    std::tm localTime{};
    localTime.tm_isdst = -1;
    std::istringstream timestampStream{timestampText};
    timestampStream >> std::get_time(&localTime, "%Y-%m-%d_%H-%M-%S");
    if (timestampStream.fail()) {
        return std::nullopt;
    }

    const std::time_t timestamp = std::mktime(&localTime);
    if (timestamp == static_cast<std::time_t>(-1)) {
        return std::nullopt;
    }

    std::ostringstream normalizedTimestamp;
    normalizedTimestamp << std::put_time(&localTime, "%Y-%m-%d_%H-%M-%S");
    if (normalizedTimestamp.str() != timestampText) {
        return std::nullopt;
    }

    return timestamp;
}

void deleteExpiredLogFiles(const std::filesystem::path& logDirectory,
                           std::chrono::system_clock::time_point now) {
    const std::time_t expirationTime = std::chrono::system_clock::to_time_t(now - logRetention);
    for (const auto& entry : std::filesystem::directory_iterator(logDirectory)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        const auto timestamp = getLogTimestamp(entry.path());
        if (timestamp.has_value() && *timestamp < expirationTime) {
            std::filesystem::remove(entry.path());
        }
    }
}

std::string_view getLevelName(LogLevel level) {
    switch (level) {
    case LogLevel::Debug:
        return "DEBUG";
    case LogLevel::Info:
        return "INFO";
    case LogLevel::Warning:
        return "WARN";
    case LogLevel::Error:
        return "ERROR";
    }
    throw std::invalid_argument("Unknown log level");
}

std::string_view getLevelColor(LogLevel level) {
    switch (level) {
    case LogLevel::Debug:
        return colorBlue;
    case LogLevel::Info:
        return colorReset;
    case LogLevel::Warning:
        return colorYellow;
    case LogLevel::Error:
        return colorRed;
    }
    throw std::invalid_argument("Unknown log level");
}

void validateStreamName(std::string_view name) {
    if (name.empty()) {
        throw std::invalid_argument("Log stream names cannot be empty");
    }
    if (name.find('/') != std::string_view::npos) {
        throw std::invalid_argument("Log stream names cannot contain '/'");
    }
}
} // namespace

Logger::Logger() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    const std::tm localTime = getLocalTime(nowTime);

    std::ostringstream fileName;
    fileName << "log_" << std::put_time(&localTime, "%Y-%m-%d_%H-%M-%S") << ".txt";

    const std::filesystem::path logDirectory{"logFile"};
    std::filesystem::create_directories(logDirectory);
    deleteExpiredLogFiles(logDirectory, now);
    logFile.open(logDirectory / fileName.str());
    if (!logFile.is_open()) {
        throw std::runtime_error("Failed to open the log file");
    }
}

LogStream::LogStream(Logger& owner, std::string streamPath)
    : logger{&owner}, path{std::move(streamPath)} {}

LogStream LogStream::Child(std::string name) const {
    return logger->CreateChildStream(path, std::move(name));
}

void LogStream::Debug(std::string_view message) const {
    logger->Log(message, LogLevel::Debug, path);
}

void LogStream::Info(std::string_view message) const {
    logger->Log(message, LogLevel::Info, path);
}

void LogStream::Warning(std::string_view message) const {
    logger->Log(message, LogLevel::Warning, path);
}

void LogStream::Error(std::string_view message) const {
    logger->Log(message, LogLevel::Error, path);
}

LogStream Logger::CreateStream(std::string name) {
    validateStreamName(name);
    std::scoped_lock lock{mutex};
    auto [streamIt, inserted] =
        streams.try_emplace(name, StreamDefinition{name, {}, 0});
    if (!inserted && !streamIt->second.parentPath.empty()) {
        throw std::logic_error("A child log stream already uses the path '" + name + "'");
    }
    return LogStream{*this, streamIt->first};
}

LogStream Logger::CreateChildStream(std::string_view parentPath, std::string name) {
    validateStreamName(name);
    std::scoped_lock lock{mutex};
    const auto parentIt = streams.find(std::string{parentPath});
    if (parentIt == streams.end()) {
        throw std::invalid_argument("Parent log stream does not exist: " +
                                    std::string{parentPath});
    }

    std::string childPath = std::string{parentPath} + '/' + name;
    auto [streamIt, inserted] = streams.try_emplace(
        childPath,
        StreamDefinition{std::move(name), std::string{parentPath}, parentIt->second.depth + 1});
    if (!inserted && streamIt->second.parentPath != parentPath) {
        throw std::logic_error("Log stream path has a different parent: " + childPath);
    }
    return LogStream{*this, std::move(childPath)};
}

std::string Logger::FormatCategory(std::string_view streamName) const {
    const auto streamIt = streams.find(std::string{streamName});
    if (streamIt == streams.end()) {
        throw std::invalid_argument("Log stream does not exist: " + std::string{streamName});
    }

    std::string category;
    for (std::size_t depth = 0; depth < streamIt->second.depth; ++depth) {
        category += "|   ";
    }
    category += streamIt->second.depth == 0 ? "+-- " : "|-- ";
    category += streamName;
    return category;
}

void Logger::Log(std::string_view message, LogLevel level, std::string_view streamName) {
    if (level == LogLevel::Debug && !inDebug) {
        return;
    }

    std::scoped_lock lock{mutex};
    const std::string timestamp = formatLogTimestamp(std::chrono::system_clock::now());
    const std::string category = FormatCategory(streamName);

    std::ostringstream line;
    line << '[' << timestamp << "] [" << std::left << std::setw(5) << getLevelName(level) << "] "
         << std::setw(static_cast<int>(categoryColumnWidth)) << category << " | " << message;

    std::ostream& console =
        level == LogLevel::Warning || level == LogLevel::Error ? std::cerr : std::cout;
    console << getLevelColor(level) << line.str() << colorReset << '\n';
    logFile << line.str() << '\n';
}

void Logger::Flush() {
    std::scoped_lock lock{mutex};
    std::cout.flush();
    std::cerr.flush();
    logFile.flush();
}

std::vector<char> Logger::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open " + filename);
    }

    const auto fileSize = file.tellg();
    if (fileSize < 0) {
        throw std::runtime_error("Failed to determine the size of " + filename);
    }

    std::vector<char> buffer(static_cast<std::size_t>(fileSize));
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    if (!file) {
        throw std::runtime_error("Failed to read " + filename);
    }

    return buffer;
}