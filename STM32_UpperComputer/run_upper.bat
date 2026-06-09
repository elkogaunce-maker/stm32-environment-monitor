@echo off
chcp 65001 >nul
set "BUNDLED_PY=C:\Users\32460\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe"

if exist "%BUNDLED_PY%" (
    "%BUNDLED_PY%" "%~dp0stm32_monitor_gui.py"
) else (
    python "%~dp0stm32_monitor_gui.py"
)

pause
