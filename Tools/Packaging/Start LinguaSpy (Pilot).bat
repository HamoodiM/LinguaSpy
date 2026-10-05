@echo off
rem LinguaSpy pilot launcher: turns on pilot mode (participant code, local learning log, field tests).
rem Nothing is sent over the network. Logs: %LOCALAPPDATA%\Personal\Saved\Telemetry
start "" "%~dp0Personal.exe" -PilotMode
