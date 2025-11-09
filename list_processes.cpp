#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include "vmmdll.h"
#include "vmm_loader.h"

int main() {
    std::cout << "==============================================" << std::endl;
    std::cout << "  DMA Process Lister" << std::endl;
    std::cout << "==============================================" << std::endl;
    std::cout << std::endl;
    
    // Load vmm.dll
    if (!LoadVmmDll()) {
        std::cout << "[ERROR] Failed to load vmm.dll!" << std::endl;
        std::cin.get();
        return 1;
    }
    
    // Initialize DMA
    std::cout << "[*] Initializing DMA..." << std::endl;
    LPCSTR argv[] = {"", "-device", "fpga"};
    VMM_HANDLE hVMM = VMMDLL_Initialize(3, argv);
    
    if (!hVMM) {
        std::cout << "[ERROR] Failed to initialize DMA!" << std::endl;
        std::cin.get();
        return 1;
    }
    
    std::cout << "[SUCCESS] DMA initialized" << std::endl << std::endl;
    
    // Get process list
    std::cout << "[*] Enumerating processes..." << std::endl << std::endl;
    
    DWORD pids[4096];
    SIZE_T cPIDs = 4096;
    
    if (!VMMDLL_PidList(hVMM, pids, &cPIDs)) {
        std::cout << "[ERROR] Failed to get process list!" << std::endl;
        std::cin.get();
        return 1;
    }
    
    std::cout << "Found " << cPIDs << " processes:" << std::endl;
    std::cout << "============================================" << std::endl << std::endl;
    
    struct ProcessInfo {
        DWORD pid;
        std::string name;
    };
    
    std::vector<ProcessInfo> processes;
    
    for (SIZE_T i = 0; i < cPIDs; i++) {
        VMMDLL_PROCESS_INFORMATION procInfo = {0};
        procInfo.magic = VMMDLL_PROCESS_INFORMATION_MAGIC;
        procInfo.wVersion = VMMDLL_PROCESS_INFORMATION_VERSION;
        SIZE_T cbProcInfo = sizeof(VMMDLL_PROCESS_INFORMATION);
        
        if (VMMDLL_ProcessGetInformation(hVMM, pids[i], &procInfo, &cbProcInfo)) {
            ProcessInfo info;
            info.pid = pids[i];
            info.name = procInfo.szName;
            processes.push_back(info);
        }
    }
    
    // Sort by name
    std::sort(processes.begin(), processes.end(), [](const ProcessInfo& a, const ProcessInfo& b) {
        return a.name < b.name;
    });
    
    // Print all processes
    for (const auto& proc : processes) {
        std::cout << "PID: " << proc.pid << "\\t" << proc.name << std::endl;
    }
    
    std::cout << std::endl << "============================================" << std::endl;
    std::cout << "Total: " << processes.size() << " processes" << std::endl;
    std::cout << std::endl << "Look for 'Pioneer' or 'Arc' in the list above!" << std::endl;
    
    UnloadVmmDll();
    
    std::cout << std::endl << "Press any key to exit..." << std::endl;
    std::cin.get();
    
    return 0;
}
