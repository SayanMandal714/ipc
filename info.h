
#ifndef INFO_H
#define INFO_H
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <iostream>
#include<string>

struct CpuStat {
    long long user, nice, system, idle, iowait, irq, softirq;
};

CpuStat readCPU() {
    std::ifstream file("/proc/stat");
    std::string cpu;
    CpuStat s;
    file >> cpu >> s.user >> s.nice >> s.system
         >> s.idle >> s.iowait >> s.irq >> s.softirq;
    return s;
}

double cpuUsage(const CpuStat& a, const CpuStat& b) {
    long idleA = a.idle + a.iowait;
    long idleB = b.idle + b.iowait;

    long totalA = a.user + a.nice + a.system + a.idle +
                  a.iowait + a.irq + a.softirq ;

    long totalB = b.user + b.nice + b.system + b.idle +
                  b.iowait + b.irq + b.softirq ;

    long totalDiff = totalB - totalA;
    long idleDiff  = idleB  - idleA;

    if (totalDiff <= 0) return 0.0;   // ⭐ prevents -nan%

    return (1.0 - (double)idleDiff / totalDiff) * 100.0;
}

//cpuload
void printCPULoad(double cpu) {
    int bars = cpu / 5;
    std::cout <<"Cpu Load : ";
    for (int i = 0; i < 20; i++)
        std::cout << (i < bars ? "▓" : "░");
    std::cout << "  " << cpu << "%\n";
}
#endif 
