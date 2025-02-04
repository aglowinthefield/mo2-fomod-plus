#pragma once
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

// Function to print log messages with colors
inline void logMessage(const LogLevel level, const std::string& message) {
    switch (level) {
    case DEBUG:
        std::cout << BLUE << "[DEBUG] " << message << RESET << std::endl;
        break;
    case INFO:
        std::cout << GREEN << "[INFO] " << message << RESET << std::endl;
        break;
    case WARN:
        std::cout << YELLOW << "[WARN] " << message << RESET << std::endl;
        break;
    case ERR:
        std::cout << RED << "[ERROR] " << message << RESET << std::endl;
        break;
    }
}
