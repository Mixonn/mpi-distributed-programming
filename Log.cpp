#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <stdio.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

class Log{
public:
    static void error(const std::string& message){
        log_color("[ERROR]" + message, ANSI_COLOR_RED);
    }

    static void warn(const std::string& message){
        log_color("[WARN]" + message, ANSI_COLOR_YELLOW);
    }

    static void debug(const std::string& message){
        log_color("[DEBUG]" + message, ANSI_COLOR_GREEN);
    }

    static void info(const std::string& message){
        log_color(message, ANSI_COLOR_RESET);
    }

private:
    static void log_color(const std::string& message, const std::string& color){
        std::cout << color << message << ANSI_COLOR_RESET << std::endl;
    }
};
