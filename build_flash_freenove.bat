@echo off
setlocal
rem Build retro-go for the Freenove FNK0104B and flash the full image.
rem Any of these can be overridden by setting them in the environment first.
if not defined IDF_TOOLS_PATH set "IDF_TOOLS_PATH=C:\Espressif"
if not defined IDF_PATH       set "IDF_PATH=C:\esp\v5.3.6\esp-idf"
if not defined RG_TOOL_TARGET set "RG_TOOL_TARGET=freenove-fnk0104b"
if not defined RG_TOOL_PORT   set "RG_TOOL_PORT=COM3"
if not defined RG_TOOL_BAUD   set "RG_TOOL_BAUD=921600"

if not exist "%IDF_PATH%\export.bat" (
    echo ESP-IDF not found at "%IDF_PATH%". Set IDF_PATH to your esp-idf folder.
    exit /b 1
)

call "%IDF_PATH%\export.bat"
if errorlevel 1 (
    echo Failed to load the ESP-IDF environment.
    exit /b 1
)

cd /d "%~dp0"
python rg_tool.py --target %RG_TOOL_TARGET% --port %RG_TOOL_PORT% --baud %RG_TOOL_BAUD% install
