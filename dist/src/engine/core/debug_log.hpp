#ifndef DEBUG_LOG_HPP
#define DEBUG_LOG_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <mutex>

static void DebugLog(const std::string& msg) {
    static std::mutex logMutex;
    std::lock_guard<std::mutex> lock(logMutex);
    std::ofstream log("crash_trace.txt", std::ios::app);
    if (log.is_open()) {
        log << msg << std::endl;
    }
    // Duplicate to console for beam_flux.log capture
    std::cout << "[DEBUG_LOG] " << msg << std::endl;
}

#endif
