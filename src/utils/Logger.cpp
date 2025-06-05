#include "Logger.h"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace X2FBX {

// Static members initialization
std::unique_ptr<Logger> Logger::instance_ = nullptr;
std::mutex Logger::mutex_;

Logger::Logger()
    : currentLevel_(LogLevel::INFO)
    , enableConsole_(true)
    , enableFile_(true)
    , logFilePath_("x2fbx_converter.log") {
}

Logger::~Logger() {
    if (logFile_.is_open()) {
        logFile_.close();
    }
}

Logger& Logger::GetInstance() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (instance_ == nullptr) {
        instance_ = std::unique_ptr<Logger>(new Logger());
    }
    return *instance_;
}

void Logger::Initialize(const std::string& logFilePath, LogLevel level) {
    Logger& logger = GetInstance();
    logger.logFilePath_ = logFilePath;
    logger.currentLevel_ = level;

    if (logger.enableFile_) {
        logger.logFile_.open(logFilePath, std::ios::out | std::ios::app);
        if (!logger.logFile_.is_open()) {
            std::cerr << "Warning: Could not open log file: " << logFilePath << std::endl;
            logger.enableFile_ = false;
        }
    }

    logger.Info("X2FBX Converter Logger initialized");
    logger.Info("Log level: " + logger.LogLevelToString(level));
}

void Logger::Log(LogLevel level, const std::string& message,
                 const std::string& file, int line) {
    if (level < currentLevel_) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    std::string formattedMessage = FormatLogMessage(level, message, file, line);

    // Console output
    if (enableConsole_) {
        if (level >= LogLevel::ERROR) {
            std::cerr << formattedMessage << std::endl;
        } else {
            std::cout << formattedMessage << std::endl;
        }
    }

    // File output
    if (enableFile_ && logFile_.is_open()) {
        logFile_ << formattedMessage << std::endl;
        logFile_.flush();
    }
}

void Logger::Debug(const std::string& message, const std::string& file, int line) {
    Log(LogLevel::DEBUG, message, file, line);
}

void Logger::Info(const std::string& message, const std::string& file, int line) {
    Log(LogLevel::INFO, message, file, line);
}

void Logger::Warning(const std::string& message, const std::string& file, int line) {
    Log(LogLevel::WARNING, message, file, line);
}

void Logger::Error(const std::string& message, const std::string& file, int line) {
    Log(LogLevel::ERROR, message, file, line);
}

void Logger::Critical(const std::string& message, const std::string& file, int line) {
    Log(LogLevel::CRITICAL, message, file, line);
}

void Logger::LogTimingInfo(const std::string& operation, double durationMs) {
    std::stringstream ss;
    ss << "TIMING: " << operation << " completed in "
       << std::fixed << std::setprecision(3) << durationMs << " ms";
    Info(ss.str());
}

void Logger::LogAnimationTiming(const std::string& animName, float originalDuration,
                               float convertedDuration, float ticksPerSecond) {
    std::stringstream ss;
    ss << "ANIMATION_TIMING: '" << animName << "' - "
       << "Original: " << std::fixed << std::setprecision(3) << originalDuration << "s, "
       << "Converted: " << convertedDuration << "s, "
       << "TicksPerSecond: " << ticksPerSecond;

    float timingError = std::abs(originalDuration - convertedDuration);
    if (timingError > 0.1f) {
        ss << " [WARNING: Timing difference of " << timingError << "s]";
        Warning(ss.str());
    } else {
        Info(ss.str());
    }
}

void Logger::LogValidationResult(const std::string& component, bool isValid,
                                const std::vector<std::string>& errors) {
    if (isValid) {
        Info("VALIDATION: " + component + " - PASSED");
    } else {
        Error("VALIDATION: " + component + " - FAILED");
        for (const auto& error : errors) {
            Error("  - " + error);
        }
    }
}

void Logger::LogProgress(const std::string& operation, int current, int total) {
    if (total > 0) {
        float percentage = (static_cast<float>(current) / total) * 100.0f;
        std::stringstream ss;
        ss << "PROGRESS: " << operation << " - "
           << current << "/" << total
           << " (" << std::fixed << std::setprecision(1) << percentage << "%)";
        Info(ss.str());
    }
}

void Logger::Flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (logFile_.is_open()) {
        logFile_.flush();
    }
    std::cout.flush();
    std::cerr.flush();
}

std::string Logger::GetCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();

    return ss.str();
}

std::string Logger::LogLevelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG:    return "DEBUG";
        case LogLevel::INFO:     return "INFO";
        case LogLevel::WARNING:  return "WARNING";
        case LogLevel::ERROR:    return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
        default:                 return "UNKNOWN";
    }
}

std::string Logger::FormatLogMessage(LogLevel level, const std::string& message,
                                    const std::string& file, int line) const {
    std::stringstream ss;

    // Timestamp
    ss << "[" << GetCurrentTimestamp() << "] ";

    // Log level
    ss << "[" << std::setw(8) << LogLevelToString(level) << "] ";

    // Message
    ss << message;

    // File and line (only for DEBUG level or if explicitly provided)
    if (level == LogLevel::DEBUG && !file.empty() && line > 0) {
        // Extract just the filename from the full path
        size_t lastSlash = file.find_last_of("/\\");
        std::string filename = (lastSlash != std::string::npos) ?
                              file.substr(lastSlash + 1) : file;
        ss << " [" << filename << ":" << line << "]";
    }

    return ss.str();
}

// TimingLogger implementation
TimingLogger::TimingLogger(const std::string& operation)
    : operation_(operation)
    , startTime_(std::chrono::high_resolution_clock::now()) {
}

TimingLogger::~TimingLogger() {
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        endTime - startTime_);
    double durationMs = duration.count() / 1000.0;

    Logger::GetInstance().LogTimingInfo(operation_, durationMs);
}

} // namespace X2FBX
