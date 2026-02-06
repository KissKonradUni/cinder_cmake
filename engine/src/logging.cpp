#include "logging.hpp"

#include "SDL3/SDL_log.h"

#include <chrono>
#include <format>

namespace echo {

std::unique_ptr<Logger> Logger::sInstance = std::make_unique<Logger>();

Logger::Logger() {
    m_FlushThread = std::thread(&Logger::threadFunc, this);
}

Logger::~Logger() {
    m_Running = false;
    m_FlushCondition.notify_all();
    if (m_FlushThread.joinable()) {
        m_FlushThread.join();
    }
}

void Logger::threadFunc() {
    static thread_local std::string timestampBuffer;
    timestampBuffer.reserve(64); // Enough for formatted timestamp
    static thread_local std::string formatBuffer;
    formatBuffer.reserve(1024 + 256); // Reserve extra space for timestamp and level

    while (m_Running) {
        std::unique_lock<std::mutex> lock(m_FlushMutex);
        m_FlushCondition.wait(lock, [this] { return !m_Running || m_FlushCursor < m_CurrentOffset; });

        while (m_FlushCursor < m_CurrentOffset) {
            formatBuffer.clear();
            const LogMessage& msg = m_LogBuffer[m_FlushCursor % m_LogBuffer.size()];

            const auto time = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
            const auto today = std::chrono::floor<std::chrono::days>(time);
            timestampBuffer.clear();
            std::format_to(
                std::back_inserter(timestampBuffer),
                "{0:%Y-%m-%d}-{1:%H:%M:%S}",
                std::chrono::year_month_day{today},
                std::chrono::hh_mm_ss{time - today}
            );

            std::format_to(
                std::back_inserter(formatBuffer),
                "[{}][{}{}{}]: {}{}\n",
                timestampBuffer,
                logLevelToColorCode(msg.level),
                logLevelToString(msg.level),
                resetColorCode(),
                msg.message,
                msg.isTruncated ? "...[TRUNCATED]" : ""
            );
            SDL_Log("%s", formatBuffer.c_str());
            // TODO: Flush to file and optional imgui console

            m_FlushCursor++;
        }
    }
}

void Logger::log(LogLevel level, const std::string& message) {
    size_t offset = sInstance->m_CurrentOffset.fetch_add(1);
    LogMessage& logMsg = sInstance->m_LogBuffer[offset % sInstance->m_LogBuffer.size()];
    logMsg.level = level;
    strncpy(logMsg.message, message.c_str(), sizeof(logMsg.message) - 1);
    logMsg.message[sizeof(logMsg.message) - 1] = '\0'; 

    if (message.size() >= sizeof(logMsg.message)) {
        logMsg.isTruncated = true;
    } else {
        logMsg.isTruncated = false;
    }

    sInstance->m_FlushCondition.notify_all();
}

};