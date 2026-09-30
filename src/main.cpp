#include <fstream>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <sstream>
#include <filesystem>
#include <cctype>

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

ll totalCPUtime(const cpuStats& cpu){
    return cpu.idle + cpu.iowait + cpu.irq + cpu.nice + cpu.softirq + cpu.steal + cpu.system + cpu.user; 
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

    }
    
    return 0;
}