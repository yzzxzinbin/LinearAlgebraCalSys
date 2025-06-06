#include "logger.h"
#include <sstream>
#include <iomanip>

Logger* Logger::instance = nullptr;

Logger::Logger() : currentLevel(LogLevel::INFO) {
    // 创建/打开日志文件
    logFile.open("k:/test program/LinearAlgebraCalSys/application.log", std::ios::out | std::ios::app);
    
    if (!logFile.is_open()) {
        std::cerr << "Failed to open log file!" << std::endl;
    }
    
    // 记录启动信息
    log(LogLevel::INFO, "Logger initialized");
}

Logger::~Logger() {
    if (logFile.is_open()) {
        log(LogLevel::INFO, "Logger shutdown");
        logFile.close();
    }
}

Logger* Logger::getInstance() {
    if (instance == nullptr) {
        instance = new Logger();
    }
    return instance;
}

std::string Logger::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string Logger::getLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR:   return "ERROR";
        case LogLevel::FATAL:   return "FATAL";
        default:                return "UNKNOWN";
    }
}

void Logger::setLogLevel(LogLevel level) {
    currentLevel = level;
}

void Logger::log(LogLevel level, const std::string& message) {
    // 如果级别低于当前设置的级别，不记录
    if (level < currentLevel) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(logMutex);
    
    if (logFile.is_open()) {
        logFile << "[" << getCurrentTime() << "] [" << getLevelString(level) << "] " 
                << message << std::endl;
        logFile.flush();
    }
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warning(const std::string& message) {
    log(LogLevel::WARNING, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::fatal(const std::string& message) {
    log(LogLevel::FATAL, message);
}
