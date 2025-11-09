# Arc Raiders Comprehensive Offset Dumper v3

Complete offset dumper for Arc Raiders using DMA hardware. Extracts all offsets needed for cheat development.

## Features

✅ **GWorld Pattern Scanning** - 4 different patterns to find GWorld  
✅ **ViewMatrix Detection** - Finds camera/view matrix for ESP  
✅ **GNames Extraction** - Object name resolution  
✅ **UE5 Structure Offsets** - Complete UE5 class hierarchy  
✅ **Game-Specific Offsets** - Player, weapon, mesh offsets  
✅ **Theia Detection** - Identifies encrypted pointers  
✅ **SDK Generation** - Generates C++ header file  

## Requirements

- **2-PC DMA Setup**:
  - Gaming PC: Runs Arc Raiders
  - Cheat PC: Has DMA card, runs this dumper
- **DMA Hardware**: ScreamerM2 or compatible
- **MemProcFS**: vmm.dll and dependencies
- **Visual Studio 2022**: For compilation

## Quick Start

### On Cheat PC:

1. **Clone the repository**:
   ```cmd
   git clone https://github.com/xmodius/ArcRaiders-Comprehensive-Dumper.git
   cd ArcRaiders-Comprehensive-Dumper
   ```

2. **Run setup script**:
   ```cmd
   setup_and_run_x64.bat
   ```

3. **Launch Arc Raiders on Gaming PC** and get into a match

4. **Press any key** to start dumping (takes 3-5 minutes)

## Output Files

- **offsets_comprehensive.txt** - Human-readable offset dump
- **Offsets_Complete.h** - C++ header file for integration

## What Gets Dumped

### Critical Offsets
- GNames (object name resolution)
- GWorld (main world object)
- ViewMatrix (camera/ESP calculations)

### UE5 Structure Offsets
- UWorld, ULevel, AActor
- APlayerState, APawn, APlayerController
- USceneComponent, APlayerCameraManager

### Game-Specific Offsets
- APlayerCharacter (health, team, mesh)
- AWeapon (ammo, recoil, spread)
- USkeletalMeshComponent (bones)
- Camera/View offsets

## Troubleshooting

### "Failed to get process list"
- Make sure Gaming PC is powered on
- Check DMA USB cable connection
- Verify DMA drivers are installed

### "GWorld not found"
- This is normal for Theia-protected games
- GWorld may be encrypted
- Manual pattern scanning or decryption needed

### "Compilation failed"
- Make sure you're using x64 Developer Command Prompt
- Check that Visual Studio 2022 is installed
- Verify cl.exe is in PATH

## Next Steps

1. **Verify GWorld** - Test if found offset is correct
2. **Test Game Offsets** - Verify health, team, etc.
3. **Implement Theia Decryption** - If GWorld is encrypted
4. **Build Main Cheat** - Use offsets in ESP/aimbot

## Notes

- Dumper must run on **Cheat PC**, not Gaming PC
- Arc Raiders must be **in a match**, not just menu
- Some offsets may need verification in-game
- Theia encryption may require additional reverse engineering

## Credits

- MemProcFS by Ulf Frisk
- Pattern scanning techniques from UnknownCheats community
