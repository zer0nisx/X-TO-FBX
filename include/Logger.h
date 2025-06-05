#pragma once

#include <string>
#include <fstream>
#include <memory>
#include <chrono>
#include <mutex>
#include <vector>

namespace X2FBX {

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    CRITICAL = 4
};

class Logger {
private:
    static std::unique_ptr<Logger> instance_;
    static std::mutex mutex_;

    std::ofstream logFile_;
    LogLevel currentLevel_;
    bool enableConsole_;
    bool enableFile_;
    std::string logFilePath_;

    Logger();

public:
    // Singleton pattern
    static Logger& GetInstance();
    static void Initialize(const std::string& logFilePath = "x2fbx_converter.log",
                          LogLevel level = LogLevel::INFO);

    // Disable copy constructor and assignment
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    ~Logger();

    // Configuration
    void SetLogLevel(LogLevel level) { currentLevel_ = level; }
    void EnableConsoleOutput(bool enable) { enableConsole_ = enable; }
    void EnableFileOutput(bool enable) { enableFile_ = enable; }

    // Logging methods
    void Log(LogLevel level, const std::string& message,
             const std::string& file = "", int line = -1);

    void Debug(const std::string& message, const std::string& file = "", int line = -1);
    void Info(const std::string& message, const std::string& file = "", int line = -1);
    void Warning(const std::string& message, const std::string& file = "", int line = -1);
    void Error(const std::string& message, const std::string& file = "", int line = -1);
    void Critical(const std::string& message, const std::string& file = "", int line = -1);

    // Special methods for timing analysis
    void LogTimingInfo(const std::string& operation, double durationMs);
    void LogAnimationTiming(const std::string& animName, float originalDuration,
                           float convertedDuration, float ticksPerSecond);
    void LogValidationResult(const std::string& component, bool isValid,
                           const std::vector<std::string>& errors);

    // Progress reporting
    void LogProgress(const std::string& operation, int current, int total);

    // Flush logs
    void Flush();

private:
    std::string GetCurrentTimestamp() const;
    std::string LogLevelToString(LogLevel level) const;
    std::string FormatLogMessage(LogLevel level, const std::string& message,
                                const std::string& file, int line) const;
};

// Convenience macros for logging with file and line info
#define LOG_DEBUG(msg) Logger::GetInstance().Debug(msg, __FILE__, __LINE__)
#define LOG_INFO(msg) Logger::GetInstance().Info(msg, __FILE__, __LINE__)
#define LOG_WARNING(msg) Logger::GetInstance().Warning(msg, __FILE__, __LINE__)
#define LOG_ERROR(msg) Logger::GetInstance().Error(msg, __FILE__, __LINE__)
#define LOG_CRITICAL(msg) Logger::GetInstance().Critical(msg, __FILE__, __LINE__)

// Timing helper class
class TimingLogger {
private:
    std::string operation_;
    std::chrono::high_resolution_clock::time_point startTime_;

public:
    explicit TimingLogger(const std::string& operation);
    ~TimingLogger();
};

// Macro for easy timing
#define TIME_OPERATION(name) TimingLogger timer(name)

} // namespace X2FBX
