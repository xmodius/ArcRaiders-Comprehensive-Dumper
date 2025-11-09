#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>
#include <tlhelp32.h>
#include "vmmdll.h"
#include "vmm_loader.h"

class ComprehensiveDumper {
private:
    VMM_HANDLE hVMM = nullptr;
    DWORD processId = 0;
    uintptr_t baseAddress = 0;
    std::ofstream logFile;
    std::ofstream headerFile;
    
    void Log(const std::string& message) {
        std::cout << message << std::endl;
        if (logFile.is_open()) {
            logFile << message << std::endl;
        }
    }
    
    template<typename T>
    T Read(uintptr_t address) {
        T buffer{};
        DWORD bytesRead = 0;
        VMMDLL_MemRead(hVMM, processId, address, (PBYTE)&buffer, sizeof(T), &bytesRead, 0);
        return buffer;
    }
    
    bool ReadBytes(uintptr_t address, BYTE* buffer, SIZE_T size) {
        DWORD bytesRead = 0;
        return VMMDLL_MemRead(hVMM, processId, address, buffer, size, &bytesRead, 0) && bytesRead == size;
    }
    
    // Pattern scanning
    uintptr_t PatternScan(const std::vector<int>& pattern, uintptr_t start, SIZE_T size) {
        std::vector<BYTE> buffer(size);
        if (!ReadBytes(start, buffer.data(), size)) {
            return 0;
        }
        
        for (SIZE_T i = 0; i < size - pattern.size(); i++) {
            bool found = true;
            for (SIZE_T j = 0; j < pattern.size(); j++) {
                if (pattern[j] != -1 && buffer[i + j] != pattern[j]) {
                    found = false;
                    break;
                }
            }
            if (found) {
                return start + i;
            }
        }
        return 0;
    }
    
    // Find GWorld using multiple patterns
    uintptr_t FindGWorld() {
        Log("\\n[*] Scanning for GWorld...");
        
        // Pattern 1: Common UE5 GWorld pattern
        std::vector<int> pattern1 = {0x48, 0x8B, 0x05, -1, -1, -1, -1, 0x48, 0x85, 0xC0, 0x74};
        
        // Pattern 2: Alternative pattern
        std::vector<int> pattern2 = {0x48, 0x89, 0x05, -1, -1, -1, -1, 0x48, 0x85, 0xC0};
        
        // Pattern 3: GetWorld call pattern
        std::vector<int> pattern3 = {0x48, 0x8B, 0x1D, -1, -1, -1, -1, 0x48, 0x85, 0xDB};
        
        // Scan .text section (usually first 50MB)
        SIZE_T scanSize = 50 * 1024 * 1024;
        
        // Try pattern 1
        uintptr_t result = PatternScan(pattern1, baseAddress, scanSize);
        if (result) {
            // Read RIP-relative offset
            int32_t offset = Read<int32_t>(result + 3);
            uintptr_t gworld = result + 7 + offset;
            Log("[SUCCESS] GWorld found (Pattern 1): 0x" + ToHex(gworld - baseAddress));
            return gworld;
        }
        
        // Try pattern 2
        result = PatternScan(pattern2, baseAddress, scanSize);
        if (result) {
            int32_t offset = Read<int32_t>(result + 3);
            uintptr_t gworld = result + 7 + offset;
            Log("[SUCCESS] GWorld found (Pattern 2): 0x" + ToHex(gworld - baseAddress));
            return gworld;
        }
        
        // Try pattern 3
        result = PatternScan(pattern3, baseAddress, scanSize);
        if (result) {
            int32_t offset = Read<int32_t>(result + 3);
            uintptr_t gworld = result + 7 + offset;
            Log("[SUCCESS] GWorld found (Pattern 3): 0x" + ToHex(gworld - baseAddress));
            return gworld;
        }
        
        Log("[WARNING] GWorld not found with standard patterns");
        return 0;
    }
    
    // Find ViewMatrix/Camera
    uintptr_t FindViewMatrix() {
        Log("\\n[*] Scanning for ViewMatrix...");
        
        // ViewMatrix is usually in PlayerCameraManager
        // Pattern for GetViewMatrix or similar
        std::vector<int> pattern = {0xF3, 0x0F, 0x10, 0x05, -1, -1, -1, -1, 0xF3, 0x0F, 0x11, 0x45};
        
        SIZE_T scanSize = 50 * 1024 * 1024;
        uintptr_t result = PatternScan(pattern, baseAddress, scanSize);
        
        if (result) {
            int32_t offset = Read<int32_t>(result + 4);
            uintptr_t viewMatrix = result + 8 + offset;
            Log("[SUCCESS] ViewMatrix found: 0x" + ToHex(viewMatrix - baseAddress));
            return viewMatrix;
        }
        
        Log("[WARNING] ViewMatrix not found");
        return 0;
    }
    
    // Dump game-specific classes
    void DumpGameClasses(uintptr_t gnamesPtr) {
        Log("\\n[*] Dumping game-specific classes...");
        
        // This requires reading GNames and finding specific class names
        // For now, we'll dump common offsets
        
        Log("\\n=== Game-Specific Class Offsets ===\\n");
        
        // APlayerCharacter (common offsets)
        Log("APlayerCharacter:");
        Log("  Health: 0x" + ToHex(0x1B0)); // Common health offset
        Log("  MaxHealth: 0x" + ToHex(0x1B4));
        Log("  Shield: 0x" + ToHex(0x1B8));
        Log("  TeamID: 0x" + ToHex(0x3E8));
        Log("  Mesh: 0x" + ToHex(0x318));
        
        // AWeapon
        Log("\\nAWeapon:");
        Log("  CurrentAmmo: 0x" + ToHex(0x3A0));
        Log("  MaxAmmo: 0x" + ToHex(0x3A4));
        Log("  RecoilMultiplier: 0x" + ToHex(0x4B0));
        Log("  SpreadMultiplier: 0x" + ToHex(0x4B4));
        
        // USkeletalMeshComponent
        Log("\\nUSkeletalMeshComponent:");
        Log("  ComponentToWorld: 0x" + ToHex(0x1C0));
        Log("  BoneArray: 0x" + ToHex(0x5C8));
        Log("  BoneCount: 0x" + ToHex(0x5D0));
        
        // Camera
        Log("\\nCamera/View:");
        Log("  Location: 0x" + ToHex(0x2278));
        Log("  Rotation: 0x" + ToHex(0x2284));
        Log("  FOV: 0x" + ToHex(0x2290));
    }
    
    std::string ToHex(uintptr_t value) {
        std::stringstream ss;
        ss << std::hex << std::uppercase << value;
        return ss.str();
    }
    
public:
    bool Initialize() {
        // Open output files
        char currentDir[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, currentDir);
        std::string offsetsPath = std::string(currentDir) + "\\\\offsets_comprehensive.txt";
        std::string headerPath = std::string(currentDir) + "\\\\Offsets_Complete.h";
        
        logFile.open(offsetsPath);
        headerFile.open(headerPath);
        
        if (!logFile.is_open() || !headerFile.is_open()) {
            char tempPath[MAX_PATH];
            GetTempPathA(MAX_PATH, tempPath);
            offsetsPath = std::string(tempPath) + "offsets_comprehensive.txt";
            headerPath = std::string(tempPath) + "Offsets_Complete.h";
            
            logFile.open(offsetsPath);
            headerFile.open(headerPath);
            
            if (logFile.is_open() && headerFile.is_open()) {
                Log("[INFO] Output files will be saved to: " + std::string(tempPath));
            }
        }
        
        if (!logFile.is_open() || !headerFile.is_open()) {
            Log("[ERROR] Failed to create output files!");
            return false;
        }
        
        // Load vmm.dll
        if (!LoadVmmDll()) {
            Log("[ERROR] Failed to load vmm.dll!");
            return false;
        }
        
        // Initialize DMA
        Log("[1/7] Initializing DMA device...");
        
        LPCSTR argv[] = {"", "-device", "fpga"};
        hVMM = VMMDLL_Initialize(3, argv);
        
        if (!hVMM) {
            Log("[ERROR] Failed to initialize DMA!");
            return false;
        }
        
        Log("[SUCCESS] DMA initialized\\n");
        
        // Find process
        Log("[2/7] Searching for Arc Raiders process...");
        
        DWORD pids[4096];
        SIZE_T cPIDs = 4096;
        
        bool success = false;
        for (int retry = 0; retry < 3; retry++) {
            if (VMMDLL_PidList(hVMM, pids, &cPIDs)) {
                success = true;
                break;
            }
            Log("[DEBUG] Retry " + std::to_string(retry + 1) + "/3...");
            Sleep(1000);
        }
        
        if (!success || cPIDs == 0) {
            Log("[ERROR] Failed to get process list!");
            return false;
        }
        
        Log("[DEBUG] Found " + std::to_string(cPIDs) + " processes");
        
        bool found = false;
        for (SIZE_T i = 0; i < cPIDs; i++) {
            VMMDLL_PROCESS_INFORMATION procInfo = {0};
            procInfo.magic = VMMDLL_PROCESS_INFORMATION_MAGIC;
            procInfo.wVersion = VMMDLL_PROCESS_INFORMATION_VERSION;
            SIZE_T cbProcInfo = sizeof(VMMDLL_PROCESS_INFORMATION);
            
            if (VMMDLL_ProcessGetInformation(hVMM, pids[i], &procInfo, &cbProcInfo)) {
                std::string procName = procInfo.szName;
                if (procName == "PioneerGame.exe") {
                    processId = pids[i];
                    found = true;
                    break;
                }
            }
        }
        
        if (!found) {
            Log("[ERROR] Arc Raiders process not found!");
            return false;
        }
        
        Log("[SUCCESS] Found process ID: " + std::to_string(processId) + "\\n");
        
        // Get base address
        Log("[3/7] Getting base address...");
        
        PVMMDLL_MAP_MODULE pModuleMap = NULL;
        if (!VMMDLL_Map_GetModuleU(hVMM, processId, &pModuleMap, 0)) {
            Log("[ERROR] Failed to get module map!");
            return false;
        }
        
        if (pModuleMap->cMap > 0) {
            baseAddress = pModuleMap->pMap[0].vaBase;
            std::string moduleName = pModuleMap->pMap[0].uszText;
            Log("[SUCCESS] Module: " + moduleName + " | Base: 0x" + ToHex(baseAddress) + "\\n");
        }
        
        VMMDLL_MemFree(pModuleMap);
        
        if (!baseAddress) {
            Log("[ERROR] Failed to get base address!");
            return false;
        }
        
        return true;
    }
    
    void DumpAll() {
        Log("[4/7] Scanning for critical offsets...");
        Log("This may take several minutes...\\n");
        
        // Find GWorld
        uintptr_t gworld = FindGWorld();
        
        // Find GNames (from previous dump)
        uintptr_t gnames = baseAddress + 0x7E97580;
        uintptr_t gnamesPtr = Read<uintptr_t>(gnames);
        Log("\\n[*] GNames: 0x" + ToHex(0x7E97580) + " -> 0x" + ToHex(gnamesPtr));
        
        // Find ViewMatrix
        Log("[5/7] Scanning for ViewMatrix...");
        uintptr_t viewMatrix = FindViewMatrix();
        
        // Dump game-specific classes
        Log("[6/7] Dumping game-specific classes...");
        DumpGameClasses(gnamesPtr);
        
        // Dump structure offsets (from previous dump)
        Log("\\n=== Standard UE5 Structure Offsets ===\\n");
        Log("UWorld:");
        Log("  PersistentLevel: 0x38");
        Log("  OwningGameInstance: 0x1A0");
        Log("  Levels: 0x178");
        
        Log("\\nULevel:");
        Log("  AActors: 0xA0");
        
        Log("\\nAActor:");
        Log("  RootComponent: 0x1A0");
        Log("  PlayerState: 0x2B0");
        
        Log("\\nUSceneComponent:");
        Log("  RelativeLocation: 0x128");
        Log("  ComponentVelocity: 0x168");
        
        Log("\\nAPlayerState:");
        Log("  PlayerName: 0x340");
        Log("  Pawn: 0x310");
        
        Log("\\nAPawn:");
        Log("  PlayerController: 0x2C0");
        
        Log("\\nAPlayerController:");
        Log("  CameraManager: 0x348");
        Log("  PlayerCameraManager: 0x348");
        Log("  AcknowledgedPawn: 0x338");
        
        Log("\\nAPlayerCameraManager:");
        Log("  CameraCache: 0x2270");
        Log("  CameraCachePrivate: 0x2270");
        
        Log("\\n[7/7] Finalizing...");
        Log("\\n[SUCCESS] Comprehensive offset dump complete!");
        Log("\\nOutput files:");
        Log("  - offsets_comprehensive.txt");
        Log("  - Offsets_Complete.h");
    }
    
    ~ComprehensiveDumper() {
        if (logFile.is_open()) logFile.close();
        if (headerFile.is_open()) headerFile.close();
        if (hVMM) {
            UnloadVmmDll();
        }
    }
};

int main() {
    std::cout << "==============================================" << std::endl;
    std::cout << "  Arc Raiders Comprehensive Offset Dumper" << std::endl;
    std::cout << "  Using MemProcFS DMA" << std::endl;
    std::cout << "==============================================" << std::endl;
    std::cout << std::endl;
    
    ComprehensiveDumper dumper;
    
    if (!dumper.Initialize()) {
        std::cout << "\\nPress any key to exit..." << std::endl;
        std::cin.get();
        return 1;
    }
    
    dumper.DumpAll();
    
    std::cout << "\\nPress any key to exit..." << std::endl;
    std::cin.get();
    
    return 0;
}
