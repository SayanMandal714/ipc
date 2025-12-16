#ifndef PRINT_H
#define PRINT_H

#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <thread>
#include <mutex>

inline std::mutex& printMutex() {
    static std::mutex m;
    return m;
}


inline void clearScreen() {
    std::cout << "\033[2J\033[H";   // ANSI clear + cursor home
}

inline void slowRefresh(int ms=0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
inline void printHeader(const std::string& title) {
    std::cout << "\n========================================================\n";
    std::cout << " " << title << "\n";
    std::cout << "========================================================\n";
}

inline void printSection(const std::string& title) {
    std::cout << "\n[" << title << "]\n";
    std::cout << "--------------------------------------------------------\n";
}

inline void printKV(const std::string& k, const std::string& v) {
    std::cout << std::left << std::setw(12) << k << ": " << v << "\n";
}

inline void printKV(const std::string& k, int v) {
    std::cout << std::left << std::setw(12) << k << ": " << v << "\n";
}

inline void printKV(const std::string& k, double v, const std::string& unit="") {
    std::cout << std::left << std::setw(12) << k << ": "
              << std::fixed << std::setprecision(2)
              << v << unit << "\n";
}

inline void printFooter() {
    std::cout << "--------------------------------------------------------\n";
}


inline void printBar(const std::string& label,
                     int value,
                     int maxValue,
                     int width = 30,
                     char fill = '#')
{
    if (maxValue <= 0) maxValue = 1;

    int filled = (value * width) / maxValue;

    std::cout << std::left << std::setw(12) << label << ": [";

    for (int i = 0; i < width; i++) {
        if (i < filled) std::cout << fill;
        else std::cout << ' ';
    }

    std::cout << "] " << value << "/" << maxValue << "\n";
}

#endif
