#include "log.hpp"

void Log::error(const std::string& message) {
    log_color("[ERROR]: " + message, ANSI_COLOR_RED);
}

void Log::warn(const std::string& message) {
    log_color("[WARN]: " + message, ANSI_COLOR_YELLOW);
}

void Log::debug(const std::string& message) {
    log_color("[DEBUG]: " + message, ANSI_COLOR_GREEN);
}

void Log::info(const std::string& message) {
    log_color("[INFO]: " + message, ANSI_COLOR_RESET);
}

void Log::info(int id, int clk, const std::string& message) {
    log_color("[INFO] id=" + std::to_string(id) + ", clk=" + std::to_string(clk) + ": " + message, ANSI_COLOR_RESET);
}

void Log::log_color(const std::string& message, const std::string& color) {
    std::cout << color << message << ANSI_COLOR_RESET << std::endl;
}

