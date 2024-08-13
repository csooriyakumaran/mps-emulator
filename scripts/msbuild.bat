@echo off

echo Building Source ---------------------------------------------------------

REM msbuild .\mps-emulator.sln /t:Build /p:configuration="Release" /verbosity:minimal /m:20 || echo ERROR in compilation && exit
msbuild .\mps-emulator.sln /t:Build /p:configuration="Debug" /verbosity:minimal /m:20 || echo ERROR in compilation && exit

echo Done Building -----------------------------------------------------------

IF NOT "%~1" == "-run" GOTO DONE

echo Running cdx     ---------------------------------------------------------
REM call .\bin\Release-windows-x86_64\mps-server\mps-server.exe 
call .\bin\Debug-windows-x86_64\mps-server\mps-server.exe 

GOTO DONE


:DONE
