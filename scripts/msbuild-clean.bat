@echo off
msbuild .\mps-emulator.sln /t:Clean /p:configuration=Debug
msbuild .\mps-emulator.sln /t:Clean /p:configuration=Release
