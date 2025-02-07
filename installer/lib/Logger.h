#pragma once
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>


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
        mLogFile.open(filePath, std::ios::out); // std::ios::app is an option for appending but dont wanna grow it forever.
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

        std::string logEntry = "[" + className + "] " + message;

        std::lock_guard lock(mMutex);
        std::ostream& out = mLogFile.is_open() ? mLogFile : std::cout;
        switch (level) {
        case DEBUG:
            out << "[DEBUG] " << message << std::endl;
            break;
        case INFO:
            out << "[INFO] " << message << std::endl;
            break;
        case WARN:
            out << "[WARN] " << message << std::endl;
            break;
        case ERR:
            out << "[ERROR] " << message << std::endl;
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