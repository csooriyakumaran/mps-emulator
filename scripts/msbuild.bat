@echo off



IF "%~1" == "debug" GOTO BUILDDEBUG
IF "%~1" == "release" GOTO BUILDRELEASE
IF "%~1" == "clean" GOTO CLEAN

:BUILDDEBUG
echo Building Source ---- Debug ----
msbuild .\mps-emulator.sln /t:Build /p:configuration="Debug" /verbosity:minimal /m:20 || echo ERROR in compilation && exit
echo Done Building ---- Debug ----
IF "%~1" == "run" GOTO RUNDEBUG
IF "%~2" == "run" GOTO RUNDEBUG
GOTO DONE

:BUILDRELEASE
echo Building Source ---- Release ----
msbuild .\mps-emulator.sln /t:Build /p:configuration="Release" /verbosity:minimal /m:20 || echo ERROR in compilation && exit
echo Done Building ---- Release ----
IF "%~2" == "run" GOTO RUNRELEASE
GOTO DONE

:RUNDEBUG
echo Running ---- Debug ----
call .\bin\Debug-windows-x86_64\mps-server\mps-server.exe 
GOTO DONE

:RUNRELEASE
echo Running ---- Release ----
call .\bin\Release-windows-x86_64\mps-server\mps-server.exe 
GOTO DONE

:CLEAN
echo Cleaning ---- Debug ----
msbuild .\mps-emulator.sln /t:Clean /p:configuration=Debug /verbosity:normal /m:20
echo Cleaning ---- Release ----
msbuild .\mps-emulator.sln /t:Clean /p:configuration=Release /verbosity:normal /m:20
GOTO DONE

:DONE
