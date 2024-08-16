# MPS-EMULATOR

## Building From Source

### Generating project files (Windows only)
```console
$ git clone --recursive https://github.com/csooriyakumaran/mps-emulator.git
$ cd mps-emulator
```
generate build files for the target compiler
```console
$ scritps\setup.bat <target>
```
e.g.:
```console
$ scritps\setup.bat vs2022
```
this will generate a vs2022 solution file in the root, and project
files in each project directory (e.g., mps-server/mps-server.vcsproj). 

To list the supported compiler targers run the setup bat file with no arguments.
or consult the premake documentation.

e.g.:
```console
$ scritps\setup.bat 
```
Atlernatively, navigate into the scripts directory and run the setup-vs2022.bat
file from file explorere (i.e., double click). 

### Building project

If msbuild is installed on your system and in your system path, run the msbuild
batch file from the root directory. 
```console
$ scritps\msbuild.bat [debug | release ] [run]
```
the script accepts a few commands. The configuration can be specified with either
debug (includes debug symbols and no optimization) and release (no syboles and O2
optimization). By default, the debug configuraiton is built. Can also optionally
specify to run the executable after the build if finished. Additional logging 
statements are printed in Debug mode. 

Alternatively, open the Visual Studio solutoin file and build/run from there. 

Build files will be added to the .\build\ directory, and the binaries will be added
to the .\bin\ directory. Aerolib is built into a library that is statically linked
into the mps-server executable. 

The executable is stand alone so can be freely copied or moved. 

### Cleaning

Intermediate and binary file can be removed by removing the .\bin\ and .\biuld\ 
directories, or by running
```console
$ scripts\msbuild.bat clean
```
This cleans both debug and release files. 

The project and solution files can be removed by running
```console
$ scripts\setup.bat clean
```

## Usage

By default, the mps server opens tcp and udp sockets on port 65432, but this can
be changed with the optional command line flag -p or --port followed by a port number. 
```console
$ mps-server.exe [ -p <port number> | --port <port number> ]
```
Once running, connect to the server as any other scanner. If connecting from the
same machine that is running the server, the ip will be 127.0.0.1 (e.g., localhost). 

For testing, commands can be issued direclty from the server console, although in debug
build, the number of logging statements that are printed makes this somewhat annoying. 
Commands issued via the consol will have their output echoed to all connected clients. 
Data will also be sent to listening clients. 

Commands can be sent to the server from clients or the console following the same
format as described in the MPS manual. That is, commands must be less than 79 
characters, and must terminate with <CR><LF> (i.e., ascii-13 ascii-10). This 
implementation requires all caps. 

e.g.:
```console
"SET RATE 850 10\r\n"
```
will send the command `SET RATE` with the arguments `850` and `10`

### Implemented commands

- `SET RATE <rate> [<output rate>]`  Note: that internal sampling is not simulated, so `output rate` just overwrites `rate` if given
- `SCAN`  Starts a simulated scan at the set rate
- `STOP`  Stops the scan
- `REBOOT` Restarts the server, restoring defaults. Command line arguments are preserved. 

