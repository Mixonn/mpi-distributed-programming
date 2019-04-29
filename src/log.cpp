#include "log.hpp"

std::string format(const std::string& type, int id, int clk, const std::string& message){
    return "[" + type + "] id="+ std::to_string(id) + ", clk=" + std::to_string(clk) + "\t{" + message + "}";
}

void Log::error(const std::string& message) {
    log_color("[ERROR]: " + message, ANSI_COLOR_RED);
}

void Log::error(int id, int clk, const std::string& message) {
    log_color(format("ERROR", id, clk, message), ANSI_COLOR_RED);
}

void Log::warn(const std::string& message) {
    log_color("[WARN]: " + message, ANSI_COLOR_YELLOW);
}

void Log::warn(int id, int clk, const std::string& message) {
    log_color(format("WARN", id, clk, message), ANSI_COLOR_YELLOW);
}

void Log::debug(const std::string& message) {
    log_color("[DEBUG]: " + message, ANSI_COLOR_GREEN);
}

void Log::debug(int id, int clk, const std::string& message) {
    log_color(format("DEBUG", id, clk, message), ANSI_COLOR_GREEN);
}

void Log::info(const std::string& message) {
    log_color("[INFO]: " + message, ANSI_COLOR_RESET);
}


void Log::info(int id, int clk, const std::string& message) {
    log_color(format("INFO", id, clk, message), ANSI_COLOR_RESET);
}

void Log::color_info(const std::string& message, const std::string& color) {
    log_color("[INFO]: " + message, color);
}

void Log::color_info(int id, int clk, const std::string& message, const std::string& color) {
    log_color("[INFO] id=" + std::to_string(id) + ", clk=" + std::to_string(clk) + ": " + message, color);
}

void Log::log_color(const std::string& message, const std::string& color) {
    std::cout << color << message << ANSI_COLOR_RESET << std::endl;
}

