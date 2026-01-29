#pragma once
#include <iostream>
#include <mutex>
#include <string>

class SafePrint {
private:
    static inline std::mutex cout_mutex_;
public:
    static void print(const std::string& message) {
        std::lock_guard<std::mutex> lock(cout_mutex_);
        std::cout << message;
    }
};

