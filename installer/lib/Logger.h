#pragma once
#include <fstream>
#include <iomanip>
#include <iostream>

// ANSI color codes
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"

// Log levels
enum LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERR = 3
};


class Logger {
public:
    static Logger& getInstance()
    {
        static Logger instance;
        return instance;
    }

    void setLogFilePath(const std::string& filePath)
    {
        std::lock_guard lock(mMutex);
        if (mLogFile.is_open()) {
            mLogFile.close();
        }
        mLogFile.open(filePath, std::ios::out | std::ios::app);
    }

    void logMessage(const LogLevel level, const std::string& message)
    {
        std::lock_guard lock(mMutex);
        std::ostream& out = mLogFile.is_open() ? mLogFile : std::cout;
        switch (level) {
        case DEBUG:
            out << BLUE << "[DEBUG] " << message << RESET << std::endl;
            break;
        case INFO:
            out << GREEN << "[INFO] " << message << RESET << std::endl;
            break;
        case WARN:
            out << YELLOW << "[WARN] " << message << RESET << std::endl;
            break;
        case ERR:
            out << RED << "[ERROR] " << message << RESET << std::endl;
            break;
        }
    }

    Logger& operator=(const Logger&) = delete;

private:
    Logger() = default;

    ~Logger()
    {
        if (mLogFile.is_open()) {
            mLogFile.close();
        }
    }

    Logger(const Logger&) = delete;

    std::ofstream mLogFile;
    std::mutex mMutex;
};