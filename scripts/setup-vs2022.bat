@echo off

pushd %~dp0\..\
build-tools\premake\premake5.exe --file=build-project.lua vs2022
popd
GOTO Done


:Done
