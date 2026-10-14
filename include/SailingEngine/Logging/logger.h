#ifndef LOGGER_H
#define LOGGER_H

#include <cstddef>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
};

class Logger;

class LogStream {
public:
    [[nodiscard]] LogStream Child(std::string name) const;

    void Debug(std::string_view message) const;
    void Info(std::string_view message) const;
    void Warning(std::string_view message) const;
    void Error(std::string_view message) const;

    [[nodiscard]] std::string_view Name() const { return path; }

private:
    friend class Logger;

    LogStream(Logger& owner, std::string streamPath);

    Logger* logger;
    std::string path;
};

class Logger {
public:
    Logger();
    ~Logger() = default;

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    [[nodiscard]] LogStream CreateStream(std::string name);
    void Log(std::string_view message, LogLevel level, std::string_view streamName);
    void Flush();
    [[nodiscard]] static std::vector<char> readFile(const std::string& filename);

private:
    friend class LogStream;

    struct StreamDefinition {
        std::string name;
        std::string parentPath;
        std::size_t depth;
    };

    [[nodiscard]] LogStream CreateChildStream(std::string_view parentPath, std::string name);
    [[nodiscard]] std::string FormatCategory(std::string_view streamName) const;

    mutable std::mutex mutex;
    std::unordered_map<std::string, StreamDefinition> streams;
    std::ofstream logFile;
};

#endif
