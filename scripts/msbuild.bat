@echo off

echo Building Source ---------------------------------------------------------

msbuild .\mps-emulator.sln /t:Build /p:configuration="Release" /verbosity:minimal /m:20 || echo ERROR in compilation && exit
REM msbuild .\mps-emulator.sln /t:Build /p:configuration="Debug" /verbosity:minimal /m:20 || echo ERROR in compilation && exit

echo Done Building -----------------------------------------------------------

IF NOT "%~1" == "-run" GOTO DONE

echo Running       ------------------------------------------------------------
call .\bin\Release-windows-x86_64\mps-server\mps-server.exe 
REM call .\bin\Debug-windows-x86_64\mps-server\mps-server.exe 

GOTO DONE


:DONE
