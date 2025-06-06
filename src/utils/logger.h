#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <chrono>
#include <mutex>

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

class Logger {
private:
    static Logger* instance;
    std::ofstream logFile;
    LogLevel currentLevel;
    std::mutex logMutex;
    
    Logger();
    ~Logger();
    
    std::string getCurrentTime();
    std::string getLevelString(LogLevel level);
    
public:
    static Logger* getInstance();
    
    void setLogLevel(LogLevel level);
    void log(LogLevel level, const std::string& message);
    
    // 便捷方法
    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    void fatal(const std::string& message);
    
    // 禁止复制和移动
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;
};

// 全局宏，使日志更容易使用
#define LOG_DEBUG(message) Logger::getInstance()->debug(message)
#define LOG_INFO(message) Logger::getInstance()->info(message)
#define LOG_WARNING(message) Logger::getInstance()->warning(message)
#define LOG_ERROR(message) Logger::getInstance()->error(message)
#define LOG_FATAL(message) Logger::getInstance()->fatal(message)
