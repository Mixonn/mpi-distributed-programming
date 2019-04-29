#pragma once
#include <iostream>
#include <string>

const std::string ANSI_COLOR_RED = "\x1b[31m";
const std::string ANSI_COLOR_GREEN = "\x1b[32m";
const std::string ANSI_COLOR_YELLOW = "\x1b[33m";
const std::string ANSI_COLOR_BLUE = "\x1b[34m";
const std::string ANSI_COLOR_MAGENTA = "\x1b[35m";
const std::string ANSI_COLOR_CYAN = "\x1b[36m";
const std::string ANSI_COLOR_RESET = "\x1b[0m";

class Log {
public:
    static void error(const std::string& message);
    static void error(int id, int clk, const std::string& message);
    static void warn(const std::string& message);
    static void warn(int id, int clk, const std::string& message);
    static void debug(const std::string& message);
    static void debug(int id, int clk, const std::string& message);
    static void info(const std::string& message);
    static void info(int id, int clk, const std::string& message);
    static void color_info(const std::string& message, const std::string& color);
    static void color_info(int id, int clk, const std::string& message, const std::string& color);


private:
    static void log_color(const std::string& message, const std::string& color);
};
