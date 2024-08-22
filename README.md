# MPS-EMULATOR

An emulated MPS-42xx pressure scanner. Includes a TCP and UDP server that accepts commands and returns data to connected clients. 

## Generating project files (Windows)

1. Clone the repository
```console
$ git clone https://github.com/csooriyakumaran/mps-emulator.git
$ cd mps-emulator
```
2. Generate build files for the target compiler
```console
$ scritps\setup.bat <ACTION>
   --- e.g. ---
$ scritps\setup.bat vs2022
```
This will generate a vs2022 solution file in the root, and project files in each project directory (e.g., mps-server/mps-server.vcsproj). The build system is [`premake`](https://premake.github.io/), since I couldn't be bothered to learn CMake. To list the all supported compiler targets and premake actions run the `scritp/setup.bat` file with no arguments, or consult the premake documentation. The following should be all that is needed for this project. 

| ACTION                   |  DESCRIPTION                                                  |
| ------------------------ | ------------------------------------------------------------- |
| `vs2022`                 | - generates visual studio solution and project files          |
| `gmake`                  | - generates Makefiles for the gcc compiler
| `clean`                  | - removes project files as well as compiled binaries          |
| `export-compile-commands`| - generates compile_commands.json files for each build config |

The `export-compile-commands` action requires the use of a custom premake module. See [`tarruda/premake-export-compile-commads`](https://github.com/tarruda/premake-export-compile-commands) for more details. 


## Building project

Compilation will depend on the selection of compiler. So far Visual studio 2022 and gcc, have been tested with the c++20 standard. [!TODO] does this actually work on both?!!


### MSBuild (vs2022) TODO(Chris) just use msbuild nativity. 

If msbuild is installed on your system and in your system path, run the msbuild batch file from the root directory. 
```console
$ scritps\msbuild.bat [CONFIGURTION] [COMMAND]
```

| CONFIGURATION            | DESCRIPTION                                                   |
| ------------------------ | ------------------------------------------------------------- |
| `release`                | Compiles without debug symbols and O2 optimization            |
| `debug`                  | Compiles with debug symbols and no optimization               |


| COMMANDS                 |                                                               |
| ------------------------ | ------------------------------------------------------------- |
| `run`                    | - runs the target config executable after compilation         |
| `clean`                  | - removes compiled and intermediate binaries                  |

e.g.:

```console
$ scritps\msbuild.bat clean
$ scritps\msbuild.bat release run
```

Alternatively, open the Visual Studio solutoin file and build/run from there. 

Build files will be added to the .\build\ directory, and the binaries will be added to the .\bin\ directory. Aerolib is built into a library that is statically linked into the mps-server executable. 

The executable is stand alone so can be freely copied or moved. 

### Cleaning

Intermediate and binary file can be removed by removing the .\bin\ and .\biuld\ directories, or by running
```console
$ scripts\msbuild.bat clean
```
This cleans both debug and release files. 

The project and solution files can be removed by running
```console
$ scripts\setup.bat clean
```
This will also remove compiled binaries. 

## USAGE
```console
$ mps-server.exe [<OPTIONS>] [<ARGUMENTS>]
```
| OPTIONS              | ARGUMENTS             | DESCRIPTION                              |
| -------------------- | --------------------- | ---------------------------------------- |
| `-p`, `--port`       |`<port-number>`        | listening port for the server.           |
| `--disable-console`  |                       | disable the local input console.         |

e.g.:
```console
$ mps-server.exe --port 23 --disable-console
```
Once running, connect to the server as any other scanner. If connecting from the same machine that is running the server, the ip will be 127.0.0.1 (i.e., localhost), otherwise use the LAN address of the machine (be sure to configure the firewall to allow TCP and UDP traffic on the specified port). 

The console runs a listening thread that waits for user input. The user can send TCP messages to all clients simply by typing them in the console and hitting enter. 

For testing, scanner commands can be issued direclty from the server console by prefixing the normal MPS commands with a `/` (e.g. `/scan`), although in the debug configuration build, the number of logging statements that are printed makes this somewhat annoying. Commands issued via the consol will have their output echoed to all connected clients. Data will also be sent to listening clients if the issued command calls for it. 

Because the console waits for user input, it will hang on shutdown until something is written to stdin. Consequently, when not using the console it is better to use the `--disable-console` flag so that the server can be remotely restarted with the `reboot` or `restart` command, or shutdown with the `shutdown` command. 

Commands can be sent to the server from clients or the console following the same format as described in the MPS manual. That is, commands must be less than 79 characters, and must terminate with <CR><LF> (i.e., ascii-13 ascii-10). This implementation requires all upper case or all lower case.

e.g.:
```console
"SET RATE 850 10\r\n"
```
will send the command `SET RATE` with the arguments `850` and `10`

### Implemented commands

| COMMANDS                            |                                 |
| ----------------------------------- | ------------------------------- |
| `VER`                               | Return the version              |
| `SET RATE <RATE> [<OUTPUT RATE>]`   | The emulator does not support  internal sampling<br> - `RATE` sets the scan rate in Hz<br> - `RATE` is simply overwritten by `OUTPUT RATE`. |
| `SCAN`                              | Start a scan                    |
| `STOP` \| `<ESC>` (TelNet only)     | Stop a scan if one is started   |
| `REBOOT` \| `RESTART`               | Restart the server              |
| `SHUTDOWN`                          | Shutdown the server             |


