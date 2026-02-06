#pragma once

#include <condition_variable>
#include <filesystem>
#include <atomic>
#include <string>
#include <thread>
#include <array>

namespace echo {

static inline std::string logFolder = std::filesystem::current_path().string() + "/logs/";

enum class LogLevel {
    Debug,   // Verbose information
    Info,    // General information
    Warning, // Potential issues
    Error,   // Errors that need attention
    Critical // Severe errors that may cause crashes
};

constexpr const char* logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info: return "INFO";
        case LogLevel::Warning: return "WARNING";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Critical: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

constexpr const char* logLevelToColorCode(LogLevel level) {
    switch (level) {
        case LogLevel::Debug: return "\033[36m"; // Cyan
        case LogLevel::Info: return "\033[32m";  // Green
        case LogLevel::Warning: return "\033[33m"; // Yellow
        case LogLevel::Error: return "\033[31m";  // Red
        case LogLevel::Critical: return "\033[35m"; // Magenta
        default: return "\033[0m"; // Reset
    }
}

constexpr const char* resetColorCode() {
    return "\033[0m";
}

struct LogMessage {
    LogLevel level;
    bool isTruncated;
    char message[1024];
};

class Logger {
protected:
    static std::unique_ptr<Logger> sInstance;

    std::array<LogMessage, 1024> m_LogBuffer{}; // ~1MB buffer
    
    std::atomic<size_t> m_CurrentOffset{0};
    size_t m_FlushCursor{0};

    std::mutex m_FlushMutex;
    std::thread m_FlushThread;
    std::atomic<bool> m_Running{true};
    std::condition_variable m_FlushCondition;

    void threadFunc();
public:
    Logger();
    ~Logger();

    // Delete copy and move
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static void log(LogLevel level, const std::string& message);
};

static inline void log(LogLevel level, const std::string& message) {
    Logger::log(level, message);
}

static inline void logDebug(const std::string& message) {
    Logger::log(LogLevel::Debug, message);
}

static inline void logInfo(const std::string& message) {
    Logger::log(LogLevel::Info, message);
}

static inline void logWarning(const std::string& message) {
    Logger::log(LogLevel::Warning, message);
}

static inline void logError(const std::string& message) {
    Logger::log(LogLevel::Error, message);
}

}; // echo
