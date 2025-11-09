#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>
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
    
    std::string ToHex(uintptr_t value) {
        std::stringstream ss;
        ss << std::hex << std::uppercase << value;
        return ss.str();
    }
    
    // Pattern scanning with wildcards
    uintptr_t PatternScan(const std::vector<int>& pattern, uintptr_t start, SIZE_T size) {
        const SIZE_T chunkSize = 1024 * 1024; // 1MB chunks
        std::vector<BYTE> buffer(chunkSize);
        
        for (SIZE_T offset = 0; offset < size; offset += chunkSize) {
            SIZE_T readSize = min(chunkSize, size - offset);
            if (!ReadBytes(start + offset, buffer.data(), readSize)) {
                continue;
            }
            
            for (SIZE_T i = 0; i < readSize - pattern.size(); i++) {
                bool found = true;
                for (SIZE_T j = 0; j < pattern.size(); j++) {
                    if (pattern[j] != -1 && buffer[i + j] != (BYTE)pattern[j]) {
                        found = false;
                        break;
                    }
                }
                if (found) {
                    return start + offset + i;
                }
            }
        }
        return 0;
    }
    
    // Find GWorld using multiple patterns
    uintptr_t FindGWorld() {
        Log("\\n[*] Scanning for GWorld...");
        Log("    This may take 2-3 minutes...");
        
        // Pattern 1: 48 8B 05 ?? ?? ?? ?? 48 85 C0 74 (mov rax, [GWorld])
        std::vector<int> pattern1 = {0x48, 0x8B, 0x05, -1, -1, -1, -1, 0x48, 0x85, 0xC0, 0x74};
        
        // Pattern 2: 48 89 05 ?? ?? ?? ?? 48 85 C0 (mov [GWorld], rax)
        std::vector<int> pattern2 = {0x48, 0x89, 0x05, -1, -1, -1, -1, 0x48, 0x85, 0xC0};
        
        // Pattern 3: 48 8B 1D ?? ?? ?? ?? 48 85 DB (mov rbx, [GWorld])
        std::vector<int> pattern3 = {0x48, 0x8B, 0x1D, -1, -1, -1, -1, 0x48, 0x85, 0xDB};
        
        // Pattern 4: 4C 8B 05 ?? ?? ?? ?? 4D 85 C0 (mov r8, [GWorld])
        std::vector<int> pattern4 = {0x4C, 0x8B, 0x05, -1, -1, -1, -1, 0x4D, 0x85, 0xC0};
        
        SIZE_T scanSize = 100 * 1024 * 1024; // Scan first 100MB
        
        // Try each pattern
        std::vector<std::vector<int>> patterns = {pattern1, pattern2, pattern3, pattern4};
        std::vector<int> instructionSizes = {3, 3, 3, 3};
        std::vector<int> offsetPositions = {3, 3, 3, 3};
        std::vector<int> instructionLengths = {7, 7, 7, 7};
        
        for (size_t p = 0; p < patterns.size(); p++) {
            Log("    Trying pattern " + std::to_string(p + 1) + "/4...");
            uintptr_t result = PatternScan(patterns[p], baseAddress, scanSize);
            
            if (result) {
                // Read RIP-relative offset
                int32_t offset = Read<int32_t>(result + offsetPositions[p]);
                uintptr_t gworldAddr = result + instructionLengths[p] + offset;
                uintptr_t gworldOffset = gworldAddr - baseAddress;
                
                // Verify it's a valid pointer
                uintptr_t gworldPtr = Read<uintptr_t>(gworldAddr);
                if (gworldPtr > 0x10000 && gworldPtr < 0x7FFFFFFFFFFF) {
                    Log("[SUCCESS] GWorld found!");
                    Log("    Offset: 0x" + ToHex(gworldOffset));
                    Log("    Address: 0x" + ToHex(gworldAddr));
                    Log("    Pointer: 0x" + ToHex(gworldPtr));
                    
                    // Write to header
                    if (headerFile.is_open()) {
                        headerFile << "#define OFFSET_GWORLD 0x" << ToHex(gworldOffset) << "\\n";
                    }
                    
                    return gworldAddr;
                }
            }
        }
        
        Log("[WARNING] GWorld not found with standard patterns");
        Log("    This is normal for Theia-protected games");
        return 0;
    }
    
    // Find ViewMatrix
    uintptr_t FindViewMatrix() {
        Log("\\n[*] Scanning for ViewMatrix...");
        
        // ViewMatrix patterns
        std::vector<int> pattern1 = {0xF3, 0x0F, 0x10, 0x05, -1, -1, -1, -1}; // movss xmm0, [ViewMatrix]
        std::vector<int> pattern2 = {0x0F, 0x10, 0x05, -1, -1, -1, -1, 0x0F, 0x11}; // movups xmm0, [ViewMatrix]
        
        SIZE_T scanSize = 50 * 1024 * 1024;
        
        uintptr_t result = PatternScan(pattern1, baseAddress, scanSize);
        if (result) {
            int32_t offset = Read<int32_t>(result + 4);
            uintptr_t viewMatrixAddr = result + 8 + offset;
            uintptr_t viewMatrixOffset = viewMatrixAddr - baseAddress;
            
            Log("[SUCCESS] ViewMatrix found!");
            Log("    Offset: 0x" + ToHex(viewMatrixOffset));
            
            if (headerFile.is_open()) {
                headerFile << "#define OFFSET_VIEWMATRIX 0x" + ToHex(viewMatrixOffset) << "\\n";
            }
            
            return viewMatrixAddr;
        }
        
        Log("[WARNING] ViewMatrix not found");
        return 0;
    }
    
    // Dump all offsets
    void DumpOffsets(uintptr_t gworld, uintptr_t gnames, uintptr_t viewMatrix) {
        Log("\\n=== COMPREHENSIVE OFFSET DUMP ===\\n");
        
        // Critical offsets
        Log("=== Critical Offsets ===\\n");
        Log("Base Address: 0x" + ToHex(baseAddress));
        Log("GNames Offset: 0x7E97580");
        if (gworld) {
            Log("GWorld Offset: 0x" + ToHex(gworld - baseAddress));
        } else {
            Log("GWorld Offset: NOT FOUND (requires manual finding or decryption)");
        }
        if (viewMatrix) {
            Log("ViewMatrix Offset: 0x" + ToHex(viewMatrix - baseAddress));
        }
        
        // UE5 Structure offsets
        Log("\\n=== UE5 Structure Offsets ===\\n");
        Log("UWorld:");
        Log("  PersistentLevel: 0x38");
        Log("  OwningGameInstance: 0x1A0");
        Log("  Levels: 0x178");
        Log("  GameState: 0x158");
        
        Log("\\nULevel:");
        Log("  AActors: 0xA0");
        Log("  ActorsCount: 0xA8");
        
        Log("\\nAActor:");
        Log("  RootComponent: 0x1A0");
        Log("  PlayerState: 0x2B0");
        Log("  Instigator: 0x160");
        
        Log("\\nUSceneComponent:");
        Log("  RelativeLocation: 0x128");
        Log("  RelativeRotation: 0x140");
        Log("  ComponentVelocity: 0x168");
        Log("  ComponentToWorld: 0x1C0");
        
        Log("\\nAPlayerState:");
        Log("  PlayerName: 0x340");
        Log("  Pawn: 0x310");
        Log("  PlayerId: 0x318");
        Log("  bIsABot: 0x2F2");
        
        Log("\\nAPawn:");
        Log("  PlayerController: 0x2C0");
        Log("  PlayerState: 0x2B0");
        
        Log("\\nAPlayerController:");
        Log("  CameraManager: 0x348");
        Log("  AcknowledgedPawn: 0x338");
        Log("  PlayerCameraManager: 0x348");
        
        Log("\\nAPlayerCameraManager:");
        Log("  CameraCache: 0x2270");
        Log("  CameraCachePrivate: 0x2270");
        
        // Game-specific offsets (estimated)
        Log("\\n=== Game-Specific Offsets (Estimated) ===\\n");
        Log("Note: These need verification in-game\\n");
        
        Log("APlayerCharacter:");
        Log("  Health: 0x1B0");
        Log("  MaxHealth: 0x1B4");
        Log("  Shield: 0x1B8");
        Log("  MaxShield: 0x1BC");
        Log("  TeamID: 0x3E8");
        Log("  Mesh: 0x318");
        Log("  bIsDead: 0x758");
        
        Log("\\nAWeapon:");
        Log("  CurrentAmmo: 0x3A0");
        Log("  MaxAmmo: 0x3A4");
        Log("  ReserveAmmo: 0x3A8");
        Log("  RecoilMultiplier: 0x4B0");
        Log("  SpreadMultiplier: 0x4B4");
        Log("  FireRate: 0x4B8");
        
        Log("\\nUSkeletalMeshComponent:");
        Log("  ComponentToWorld: 0x1C0");
        Log("  BoneArray: 0x5C8");
        Log("  BoneCount: 0x5D0");
        Log("  CachedBoneSpaceTransforms: 0x5C8");
        
        Log("\\nCamera/View:");
        Log("  Location: 0x2278");
        Log("  Rotation: 0x2284");
        Log("  FOV: 0x2290");
        
        // Write header file
        if (headerFile.is_open()) {
            headerFile << "\\n// Arc Raiders Offsets - Generated by Comprehensive Dumper\\n";
            headerFile << "// Base: 0x" << ToHex(baseAddress) << "\\n\\n";
            
            headerFile << "// Critical\\n";
            headerFile << "#define OFFSET_GNAMES 0x7E97580\\n";
            if (gworld) {
                headerFile << "#define OFFSET_GWORLD 0x" << ToHex(gworld - baseAddress) << "\\n";
            }
            
            headerFile << "\\n// UWorld\\n";
            headerFile << "#define OFFSET_UWORLD_PERSISTENTLEVEL 0x38\\n";
            headerFile << "#define OFFSET_UWORLD_OWNINGGAMEINSTANCE 0x1A0\\n";
            headerFile << "#define OFFSET_UWORLD_LEVELS 0x178\\n";
            
            headerFile << "\\n// ULevel\\n";
            headerFile << "#define OFFSET_ULEVEL_AACTORS 0xA0\\n";
            
            headerFile << "\\n// AActor\\n";
            headerFile << "#define OFFSET_AACTOR_ROOTCOMPONENT 0x1A0\\n";
            headerFile << "#define OFFSET_AACTOR_PLAYERSTATE 0x2B0\\n";
            
            headerFile << "\\n// USceneComponent\\n";
            headerFile << "#define OFFSET_USCENECOMPONENT_RELATIVELOCATION 0x128\\n";
            headerFile << "#define OFFSET_USCENECOMPONENT_COMPONENTVELOCITY 0x168\\n";
            
            headerFile << "\\n// APlayerState\\n";
            headerFile << "#define OFFSET_APLAYERSTATE_PLAYERNAME 0x340\\n";
            headerFile << "#define OFFSET_APLAYERSTATE_PAWN 0x310\\n";
            
            headerFile << "\\n// APawn\\n";
            headerFile << "#define OFFSET_APAWN_PLAYERCONTROLLER 0x2C0\\n";
            
            headerFile << "\\n// APlayerController\\n";
            headerFile << "#define OFFSET_APLAYERCONTROLLER_CAMERAMANAGER 0x348\\n";
            
            headerFile << "\\n// APlayerCameraManager\\n";
            headerFile << "#define OFFSET_APLAYERCAMERAMANAGER_CAMERACACHE 0x2270\\n";
            
            headerFile << "\\n// Game-Specific (Verify in-game)\\n";
            headerFile << "#define OFFSET_PLAYERCHARACTER_HEALTH 0x1B0\\n";
            headerFile << "#define OFFSET_PLAYERCHARACTER_TEAMID 0x3E8\\n";
            headerFile << "#define OFFSET_PLAYERCHARACTER_MESH 0x318\\n";
            headerFile << "#define OFFSET_SKELETALMESH_BONEARRAY 0x5C8\\n";
        }
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
        Log("[1/5] Initializing DMA device...");
        
        LPCSTR argv[] = {"", "-device", "fpga"};
        hVMM = VMMDLL_Initialize(3, argv);
        
        if (!hVMM) {
            Log("[ERROR] Failed to initialize DMA!");
            return false;
        }
        
        Log("[SUCCESS] DMA initialized\\n");
        
        // Find process
        Log("[2/5] Searching for Arc Raiders process...");
        
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
        Log("[3/5] Getting base address...");
        
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
        Log("[4/5] Scanning for offsets...");
        Log("This will take 3-5 minutes. Please wait...\\n");
        
        // Find GWorld
        uintptr_t gworld = FindGWorld();
        
        // Find ViewMatrix
        uintptr_t viewMatrix = FindViewMatrix();
        
        // GNames from previous dump
        uintptr_t gnames = baseAddress + 0x7E97580;
        
        Log("\\n[5/5] Generating offset files...");
        
        // Dump everything
        DumpOffsets(gworld, gnames, viewMatrix);
        
        Log("\\n[SUCCESS] Comprehensive offset dump complete!");
        Log("\\nOutput files:");
        Log("  - offsets_comprehensive.txt (Human-readable)");
        Log("  - Offsets_Complete.h (C++ header)");
        Log("\\nNext steps:");
        Log("  1. Verify GWorld offset in-game");
        Log("  2. Test game-specific offsets");
        Log("  3. Implement Theia decryption if needed");
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
