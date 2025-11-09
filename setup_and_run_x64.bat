@echo off
echo ========================================
echo Arc Raiders Comprehensive Dumper - Setup
echo ========================================
echo.

REM Force x64 environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

echo [1/3] Copying DMA DLLs...
echo.

REM Copy DMA DLLs from common locations
if exist "C:\Users\enfui\Desktop\lones-DMA-speed-test-main\lones-DMA-speed-test-main\*.dll" (
    copy "C:\Users\enfui\Desktop\lones-DMA-speed-test-main\lones-DMA-speed-test-main\*.dll" . >nul 2>&1
    echo [SUCCESS] DMA DLLs copied!
) else (
    echo [WARNING] DMA DLLs not found in default location
    echo Please copy vmm.dll and dependencies to this folder manually
)

echo.
echo [2/3] Compiling comprehensive dumper (x64)...
echo.

cl.exe /EHsc /std:c++17 /O2 /Fe:ArcRaiders_Comprehensive_Dumper.exe main.cpp Advapi32.lib Shell32.lib >nul 2>&1

if %ERRORLEVEL% EQU 0 (
    echo [SUCCESS] Compilation complete!
) else (
    echo [ERROR] Compilation failed!
    pause
    exit /b 1
)

echo.
echo [3/3] Ready to run!
echo.
echo ========================================
echo IMPORTANT:
echo 1. Launch Arc Raiders on Gaming PC
echo 2. Get into a match (not just menu)
echo 3. Wait 15 seconds in the match
echo 4. Press any key to run the dumper
echo.
echo This will take 3-5 minutes to complete.
echo ========================================
echo.
pause

echo.
echo Running comprehensive dumper...
echo.

ArcRaiders_Comprehensive_Dumper.exe

pause
