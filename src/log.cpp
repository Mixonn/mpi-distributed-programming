#include "log.hpp"

std::string format(const std::string& type, int id, int clk, const std::string& message){
    return "[" + type + "] id="+ std::to_string(id) + ", clk=" + std::to_string(clk) + "\t{" + message + "}";
    fflush(stdout);
}

void Log::error(const std::string& message) {
    log_color("[ERROR]: " + message, ANSI_COLOR_RED);
    fflush(stdout);
}

void Log::error(int id, int clk, const std::string& message) {
    log_color(format("ERROR", id, clk, message), ANSI_COLOR_RED);
    fflush(stdout);
}

void Log::warn(const std::string& message) {
    log_color("[WARN]: " + message, ANSI_COLOR_YELLOW);
    fflush(stdout);
}

void Log::warn(int id, int clk, const std::string& message) {
    log_color(format("WARN", id, clk, message), ANSI_COLOR_YELLOW);
    fflush(stdout);
}

void Log::debug(const std::string& message) {
    log_color("[DEBUG]: " + message, ANSI_COLOR_GREEN);
    fflush(stdout);
}

void Log::debug(int id, int clk, const std::string& message) {
    log_color(format("DEBUG", id, clk, message), ANSI_COLOR_GREEN);
    fflush(stdout);
}

void Log::info(const std::string& message) {
    log_color("[INFO]: " + message, ANSI_COLOR_RESET);
    fflush(stdout);
}


void Log::info(int id, int clk, const std::string& message) {
    log_color(format("INFO", id, clk, message), ANSI_COLOR_RESET);
    fflush(stdout);
}

void Log::color_info(const std::string& message, const std::string& color) {
    log_color("[INFO]: " + message, color);
    fflush(stdout);
}

void Log::color_info(int id, int clk, const std::string& message, const std::string& color) {
    log_color("[INFO] id=" + std::to_string(id) + ", clk=" + std::to_string(clk) + ": " + message, color);
    fflush(stdout);
}

void Log::log_color(const std::string& message, const std::string& color) {
    std::cout << color << message << ANSI_COLOR_RESET << std::endl;
    fflush(stdout);
}

