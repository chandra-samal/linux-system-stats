#include <fstream>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>

using namespace std;
using ll = long long;


struct cpuStats{
    ll user;
    ll nice;
    ll system;
    ll idle;
    ll iowait;
    ll irq;
    ll softirq;
    ll steal;
};

cpuStats getStats() {
    ifstream file("/proc/stat");

    if (!file) {
        throw runtime_error("Failed to open /proc/stat\n");
    }

    string cpu;
    cpuStats stats{};

    file >> cpu >> stats.user >> stats.nice
        >> stats.system >> stats.idle >> stats.iowait >> stats.irq
        >> stats.softirq >> stats.steal;
        
    return stats;
}

ll totalCPUtime(cpuStats& cpu){
    return cpu.idle + cpu.iowait + cpu.irq + cpu.nice + cpu.softirq + cpu.steal + cpu.system + cpu.user; 
}

int main() {
    cpuStats first = getStats();
    this_thread::sleep_for(chrono::milliseconds(100));
    cpuStats second = getStats();

    ll idle1 = first.idle + first.iowait;
    ll idle2 = second.idle + second.iowait;

    ll total1 = totalCPUtime(first);
    ll total2 = totalCPUtime(second);
    
    ll totalDelta = total2 - total1;
    ll idleDelta = idle2 - idle1;

    double cpuUsage = 100.0*(totalDelta-idleDelta)/totalDelta;

    cout << "CPU usage: " << cpuUsage << '\n';

    return 0;
}