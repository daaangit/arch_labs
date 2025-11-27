#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <iomanip>

class Logger {
private:
    std::ofstream logFile;
    
public:
    Logger() {
        logFile.open("client.log", std::ios::app);
    }
    
    ~Logger() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }
    
    static std::string getCurrentTime() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
    
    void log(const std::string& message) {
        std::string logEntry = "[" + getCurrentTime() + "] INFO: " + message + "\n";
        std::cout << logEntry;
        if (logFile.is_open()) {
            logFile << logEntry;
            logFile.flush();
        }
    }
    
    void error(const std::string& message) {
        std::string logEntry = "[" + getCurrentTime() + "] ERROR: " + message + "\n";
        std::cerr << logEntry;
        if (logFile.is_open()) {
            logFile << logEntry;
            logFile.flush();
        }
    }
};

#endif
