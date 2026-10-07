#include <fstream>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <sstream>
#include <filesystem>
#include <cctype>
#include <algorithm>
#include <unordered_map>

using namespace std;
namespace fs = std::filesystem;
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

struct loadAvg{
    double oneMin;
    double fiveMin;
    double fifteenMin;
};

struct memStats{
    ll total;
    ll available;
    ll used;
    double usage;
};

struct processStats{
    ll pid;
    string name;
    ll memory;
    double cpuUsage;
};

loadAvg getLoadAvg() {
    ifstream file("/proc/loadavg");

    if (!file) {
        throw runtime_error("Failed to open /proc/loadavg\n");
    }

    loadAvg load{};
    file >> load.oneMin >> load.fiveMin >> load.fifteenMin;

    return load;
}

memStats getMemoryStats() {
    ifstream file("/proc/meminfo");

    if (!file) {
        throw runtime_error("Failed to open /proc/meminfo\n");
    }

    memStats stats{};
    string line;
    string a;
    while(getline(file, line)) {
        if (line.starts_with("MemTotal")) {
            stringstream ss(line);
            ss >> a >> stats.total;
        }
        else if (line.starts_with("MemAvailable")) {
            stringstream ss(line);
            ss >> a >> stats.available;
        }
    }
    stats.used = stats.total - stats.available;
    stats.usage = 100.0*stats.used/stats.total;

    return stats;
}

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

processStats getMostMemoryConsumingProcess() {
    processStats process{};
    for (const auto& current: fs::directory_iterator("/proc")) {
        string pid = current.path().filename().string();
        bool isProcess = true;
        for(char c:pid) {
            if (!isdigit(c)) {
                isProcess = false;
                break;
            }
        }
        if (!isProcess) continue;
        ifstream file("/proc/" + pid + "/status");
        if (!file) continue;

        string line;
        ll mem=0;
        string a;        
        while(getline(file, line)){
            if (line.starts_with("VmRSS")){
                stringstream ss(line);
                ss >> a >> mem;
                if(mem>process.memory) {
                    ifstream name("/proc/" + pid + "/comm");
                    if(!name) continue;
                    process.memory = mem;
                    process.pid = stoll(pid);
                    name >> process.name;
                }
            }
        }
    }
    return process;
}

ll getProcessTotalCPUtime(string pid) {    
    ifstream file("/proc/" + pid + "/stat");
    ll utime, stime;
    string line;
    string dummy;
    getline(file, line);
    int r = line.rfind(')');
    line.erase(0, r+1);
    stringstream ss(line);
    for(int i = 3; i<=15; i++){
        if (i==14) ss >> utime;
        else if (i==15) ss >> stime;
        else ss >> dummy;
    }
    return utime+stime;
}

ll totalCPUtime(const cpuStats& cpu){
    return cpu.idle + cpu.iowait + cpu.irq + cpu.nice + cpu.softirq + cpu.steal + cpu.system + cpu.user; 
}

processStats getMostCPUConsumingProcess() {
    processStats process{};
    process.cpuUsage = 0;

    cpuStats first = getStats();
    unordered_map<string, ll> snapshot1;

    for (const auto& current : fs::directory_iterator("/proc")) {
        string pid = current.path().filename().string();
        bool isProcess = true;
        for (char c : pid) {
            if (!isdigit(c)) {
                isProcess = false;
                break;
            }
        }
        if (!isProcess)
            continue;
        snapshot1[pid] = getProcessTotalCPUtime(pid);
    }

    this_thread::sleep_for(chrono::milliseconds(100));

    cpuStats second = getStats();
    unordered_map<string, ll> snapshot2;

    for (const auto& current : fs::directory_iterator("/proc")) {
        string pid = current.path().filename().string();
        bool isProcess = true;
        for (char c : pid) {
            if (!isdigit(c)) {
                isProcess = false;
                break;
            }
        }
        if (!isProcess) continue;

        snapshot2[pid] = getProcessTotalCPUtime(pid);
    }

    ll totalDelta = totalCPUtime(second) - totalCPUtime(first);

    for (const auto& [pid, currentCPU] : snapshot2) {
        auto previous = snapshot1.find(pid);
        if (previous == snapshot1.end()) continue;

        ll processDelta = currentCPU - previous->second;
        double usage = 100.0 * processDelta / totalDelta;
        if (usage > process.cpuUsage) {
            ifstream file("/proc/" + pid + "/comm");
            if (!file) continue;

            file >> process.name;
            process.cpuUsage = usage;
            process.pid = stoll(pid);
        }
    }

    return process;
}


int main() {
    while (true) {
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
        
        loadAvg load = getLoadAvg();
        memStats memStat = getMemoryStats();
        
        // CPU usage
        cout << "CPU usage: " << cpuUsage << "%\n";
        
        // Load Averages
        cout << "One-minute avg load: " << load.oneMin;
        cout << " Five-minute avg load: " << load.fiveMin;
        cout << " Fifteen-minute avg load: " << load.fifteenMin << '\n';
        
        // Memory Statistics
        cout << "Total RAM: " << memStat.total << " KB\n";
        cout << "Available RAM: " << memStat.available << " KB\n";
        cout << "Used RAM: " << memStat.used << " KB\n";
        cout << "RAM usage: " << memStat.usage << "%\n";

        // Most memory consuming process
        processStats mostMemoryProcess = getMostMemoryConsumingProcess();
        cout << "Most memory consuming process: " << mostMemoryProcess.name << '\n';
        cout << "PID of the process: " << mostMemoryProcess.pid << '\n';
        cout << "Memory consumed by the Process: " << mostMemoryProcess.memory << " KB\n";

        // Most CPU consuming process
        processStats mostCPUConsumingProcess = getMostCPUConsumingProcess();
        cout << "Most CPU consuming process: " << mostCPUConsumingProcess.name << '\n';
        cout << "PID of the process: " << mostCPUConsumingProcess.pid << '\n';
        cout << "CPU\% usage of the process: " << mostCPUConsumingProcess.cpuUsage << '\n';

    }
    
    return 0;
}