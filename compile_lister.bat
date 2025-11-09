@echo off
echo Compiling process lister...
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cl.exe /EHsc /std:c++17 /O2 /Fe:list_processes.exe list_processes.cpp Advapi32.lib Shell32.lib
if %ERRORLEVEL% EQU 0 (
    echo [SUCCESS] Compiled!
    echo.
    echo Run: list_processes.exe
) else (
    echo [ERROR] Compilation failed!
)
pause
