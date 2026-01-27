#pragma once
#include <fstream>
#include <iostream>
#include <mutex>
#include <regex>


// Log levels
enum LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERR = 3
};

// Convert LogLevel to string
inline const char* logLevelToString(const LogLevel level) {
    switch (level) {
    case DEBUG: return "DEBUG";
    case INFO:  return "INFO";
    case WARN:  return "WARN";
    case ERR:   return "ERROR";
    default:    return "UNKNOWN";
    }
}


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
        mLogFile.open(filePath, std::ios::out);
        // std::ios::app is an option for appending but dont wanna grow it forever.
    }

    void setDebugMode(bool debug)
    {
        std::lock_guard lock(mMutex);
        mDebugMode = debug;
    }

    void logMessage(const LogLevel level, const std::string& message)
    {

#if defined(__GNUC__) || defined(__clang__)
                std::string functionName = __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
        std::string functionName = __FUNCSIG__;
#else
                std::string functionName = "UnknownFunction";
#endif

        std::regex classNameRegex(R"((\w+)::\w+\()");
        std::smatch match;
        std::string className = "UnknownClass";

        if (std::regex_search(functionName, match, classNameRegex) && match.size() > 1) {
            className = match.str(1);
        }

        std::lock_guard lock(mMutex);
        
        auto writeLog = [&](std::ostream& stream) {
            switch (level) {
            case DEBUG:
                stream << "[DEBUG] " << message << std::endl;
                break;
            case INFO:
                stream << "[INFO] " << message << std::endl;
                break;
            case WARN:
                stream << "[WARN] " << message << std::endl;
                break;
            case ERR:
                stream << "[ERROR] " << message << std::endl;
                break;
            }
        };

        if (mLogFile.is_open()) {
            writeLog(mLogFile);
        }
        
        writeLog(std::cout);
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
#if !defined(NDEBUG) || defined(CMAKE_BUILD_TYPE_RELWITHDEBINFO)
    bool mDebugMode = true;   // Auto-enable in debug/RelWithDebInfo builds
#else
    bool mDebugMode = false;  // Disable in release builds
#endif
};