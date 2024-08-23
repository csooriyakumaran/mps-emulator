@echo off

IF "%~1" == "" GOTO PrintHelp

build-tools\premake\premake5.exe --file=build-project.lua %1

IF NOT "%~1" == "export-compile-commands" GOTO Done

IF NOT EXIST .\build\ mkdir .\build
mklink build\compile_commands.json compile_commands\debug.json

GOTO Done

:PrintHelp

echo.
echo Usage: 'setup.bat ACTION'
echo.
echo ACTIONS:
echo   clean             Remove all binaries and intermediate binaries and project files.
echo   codelite          Generate CodeLite project files
echo   gmake             Generate GNU makefiles for Linux
echo   vs2015            Generate Visual Studio 2015 project files
echo   vs2017            Generate Visual Studio 2017 project files
echo   vs2019            Generate Visual Studio 2019 project files
echo   vs2022            Generate Visual Studio 2022 project files
echo   xcode4            Generate Apple Xcode 4 project files

GOTO Done

:Done
