# MPS-EMULATOR

## Generating project files (Windows only)
```console
$ git clone --recursive https://github.com/csooriyakumaran/mps-emulator.git
$ cd mps-emulator
```
build for the target compiler
```console
$ scritps\setup.bat <target>
```
e.g.:
```console
$ scritps\setup.bat vs2022
```
this will generate a vs2022 solution file in the root, and project
files in each project directory (e.g., mps-server/mps-server.vcsproj)

for supported compiler targers run the setup bat file with no arguments.
```console
$ scritps\setup.bat 
```
Atlernatively, navigate into the scripts directory and run the 
setup-vs2022.bat file from file explorere (i.e., double click). 

## Building project

If msbuild is installed on your system and in your system path:
```console
$ scritps\msbuild.bat
```
will run msbuild.executable on mps-emulator.sln, by default building the debug configuration. 
The optional -run flag can be used to run the executable after compliation. 


