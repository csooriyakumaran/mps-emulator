# MPS-EMULATOR

An emulated MPS-42xx pressure scanner.

The emulator starts a TCP server on the default port 23 that accepts connections from multiple clients, and handles commands following the MPS format on a client-by-client basis.

Scan data are generated psuedo-randomly following a normal distrubution with a standard deviation of about 0.35 Pa. Scan data are transmitted to the requesting client via over a UDP stream (the udp data streams from port 23). Since this is primarily intended to be run from the same machine as the target data acquisition software, the default udp port target must be different from the port which sends data. Consequently, the default target port is 24. This can be changed using the `SET IPUDP <ip> <port>` command. To better simulate connecting to a physical scanner, this emulator could be run from inside a container, vm, or separate machine.  

## Generating project files

1. Clone the repository
```powershell
git clone https://github.com/csooriyakumaran/mps-emulator.git
cd mps-emulator
```
2. Generate build files for the target compiler
```powershell
scripts/setup.bat [ACTION]
```
e.g:
```powershell
scripts/setup.bat vs2022
```
This will generate a vs2022 solution file in the root, and project files in each project directory (e.g., mps-server/mps-server.vcsproj). The build system is [`premake`](https://premake.github.io/), since I couldn't be bothered to learn CMake. The premake binary is included in `./build-tools/premake/` along with the license. To list the all supported compiler targets and premake actions run the `scritp/setup.bat` file with no arguments, or consult the premake [`docs`](https://premake.github.io/docs/). The following should be all that is needed for this project. 

| ACTION                   |  DESCRIPTION                                                      |
| ------------------------ | ----------------------------------------------------------------- |
| `vs2022`                 | - generates visual studio solution and project files              |
| `gmake`                  | - generates gnu Makefiles for the gcc compiler (i.e., if on linux) **NOT FULLY TESTED**|
| `clean`                  | - removes project files as well as compiled binaries              |
| `export-compile-commands`| - generates compile_commands.json files for each build config     |

The `export-compile-commands` action requires the use of a custom premake module. See [`tarruda/premake-export-compile-commads`](https://github.com/tarruda/premake-export-compile-commands) for more details. This is only useful when using an IDE/editor that requires an external lsp client (specifically [`clangd`](https://clangd.llvm.org/)) to serve language features like auto-completion, go-to-definitions, etc. 


## Building the project

Compilation will depend on the selection of compiler. So far Visual studio 2022 and to a lesser extent gcc have been tested with the c++20 standard.

Intermediate binaries (like object files) are genearted in the `./build/` directory, and the binaries will be added to the `./bin/` directory. Aerolib is compiled into a library that is statically linked into the mps-server executable. 

The executable is stand alone so can be freely copied or moved. TBD: eventually scanner configurations will be saved to files, mirroring the approach taken by the Scanivalve scanners.

### Visual Studio (vs2022)

Generate the Visual Studio solution and prject files.
```powershell
scripts/setup.bat vs2022
```
Open the Visual Studio solution file and build/run the desired configuration from there. 

### MSBuild (vs2022)

Generate the Visual Studio solution and prject files.
```powershell
scripts/setup.bat vs2022
```
If msbuild is installed on your system and in your system path, run the msbuild batch file from the root directory. 
```powershell
scripts/msbuild.bat [CONFIGURTION] [COMMAND]
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
```powershell
 scripts/msbuild.bat release run
```
Intermediate and binary file can be removed by removing the .\bin\ and .\biuld\ directories, or by running
```powershell
 scripts/msbuild.bat clean
```
This cleans both debug and release files. 

### Make (gmake \| gmake2)

#### GNU on Windows
For a Windows development enviroment that uses the gnu toolchain (e.g., [`MSYS2`](https://www.msys2.org/) or [`mingw`](https://www.mingw-w64.org/)), the premake action `gmake` or `gmake2` can be used (instead of `vs2022`). This will generate makefiles that can be compiled using the [`gnu make`](https://www.gnu.org/software/make/) executable. 
```powershell
 scripts\setup.bat gmake
 make config=release
```

#### Linux
Only limited testing has been done in a linux enviromenent using [`wsl`](https://learn.microsoft.com/en-us/windows/wsl/install). TCP communication seems to work fine but the sever does not respect the requested port number. UDP streaming is also not working. 

Download the appropriate pre-built [`premake`](https://premake.github.io/download/) binary for your OS. 

Generate the Makefiles, build, and run. 
```sh
 ./path-to-premake-for-linux/premake5 --file=build-project.lua gmake
 make config=release
 ./bin/Release-linux-x86_64/mps-server/mps-server --disable-console --port 5888
```

### Cleaning

The project and solution files can be removed by running
```powershell
 scripts\setup.bat clean
```
This will also remove compiled binaries. 

## USAGE
```powershell
 mps-server.exe [<OPTIONS>] [<ARGUMENTS>]
```
| OPTIONS              | ARGUMENTS             | DESCRIPTION                                |
| -------------------- | --------------------- | ------------------------------------------ |
| `-p`, `--port`       |`<port-number>`        | listening port for the server. (default 23)|
| `--disable-console`  |                       | disable the local input console.           |

e.g.:
```powershell
 mps-server.exe --disable-console --port 1234 
```
Once running, connect to the server as any other scanner. If connecting from the same machine that is running the server, the ip will be 127.0.0.1 (i.e., localhost), otherwise use the LAN address of the machine (be sure to configure the firewall to allow TCP and UDP traffic on the specified port). 

### Local Console

If not disabled, a local console runs on a separate listening thread that waits for user input through stdin. The user can send TCP messages to all clients simply by typing them in the console and hitting enter. 

For testing, scanner commands can be issued direclty from the server console by prefixing the normal MPS commands with a `/` (e.g. `/set fps 300`), although in the debug configuration build, the number of logging statements that are printed makes this somewhat annoying. Commands issued via the console will have their output echoed to all connected clients. Scan requests made from the console will result in data being streamed to all listening clients.

Because the console waits for user input, it will hang on reboot/shutdown until user input is written to stdin. Consequently, when not testing with the console it is better to use the `--disable-console` flag so that the server can be remotely restarted with the `reboot` or `restart` command, or stopped with the `shutdown` command. 

### Command Formatting

Commands can be sent to the server from clients (or the console if prefixed by `/`) following the same format as described in the MPS manual. That is, commands must be less than 79 characters, and must terminate with \<CR\>\<LF\> (i.e., ascii-13 ascii-10). This implementation requires all upper case or all lower case.

e.g.:
```console
> "SET RATE 850 10\r\n"
```
will send the command `SET RATE` with the arguments `850` and `10`

To maintain compatibility with the ScanTel terminal emulator, which uses the TelNet protocol, the server accepts commands that are issued one character at a time (including backspaces to remove characters) until a carriage return is sent. 

### Commands and Syntax

Below are listed the commands which must be sent as shown and temrinated with `<CR><LF>`. Argument parameters are shown inside `< >` and optional parameters are inclosed in `[ ]`.  The following exceptions to this are special characters:

- `<BS>`: Backspace ( `0x08` )
- `<TAB>`: Horizontal tab ( `0x09` )
- `<LF>`: Line feed ( `0x0A` )
- `<CR>`: Carriage retruen ( `0x0D` )
- `<ESC>`: Escape ( `0x1B` )

Equivalent or alias commands are separated by `|`.

Consult the MPS manual for more detailed descriptions of each command, including examples. 

> [!IMPORTANT] 
> Not all commands are implemented (e.g., no FTP server is emulated)

***GENERAL COMMANDS***

| SYNTAX                              | DESCRIPTION                           |
| ----------------------------------- | ------------------------------------- |
| `VER`                               | Return the version                    |
| `REBOOT` \| `RESTART`               | Restart the server after closing all connections |
| `SHUTDOWN`                          | Shutdown the server                   |
| `STOP` \| `<ESC>` (TelNet only)     | Stop a scan if one is started         |
| `STATUS`                            | Returns the status. e.g.:<br> - `STATUS: READY`<br> - `STATUS: SCAN`<br> - `STATUS: CALZ`|
| `CALZ`                              | Start a zero-cal (in the emulator this just resets the normal distrubution mean of the scan sampler to zero)                          |
| `VALVESTATE`                        | Returns the valve state (always `PX` for the emulator) ***NOT IMPLEMENTED***                         |
| `TRIG` \| `<TAB>`                     | Returns one frame of data (requires  `SET TRIG 1`) ***NOT IMPLEMENTED***                         |
| `TREAD [<MODULE>]`                 | Returns the (simulated) module temperature on all or requested module (from 1 to 8). ***NOT IMPLEMENTED***|
| `SAVE [<cfg>]`                        | Save the configuration (not implemented) |
| `SCAN`                              | Start a scan                          |

***LIST COMMANDS***

| SYNTAX                              | DESCRIPTION                           |
| ----------------------------------- | ------------------------------------- |
| `LIST S`                            | Returns the scan settings             |
| `LIST UDP`                          | Returns the UDP settings              |

***SET COMMANDS***

| SYNTAX                              | DESCRIPTION                           |
| ----------------------------------- | ------------------------------------- |
| `SET RATE <RATE> [<OUTPUT RATE>]`   | Set the sampling and output rate of the scanner. <br> - `RATE` internal scan rate in Hz<br> - `OUTPUT RATE` rate at which data are sent. Defaults to `RATE` if ommitted. |
| `SET ENUDP <OPT>`                   | `0`: Disable UDP<br>`1`: Enable UDP   |
| `SET IPUDP <IP> <PORT>`             | Set the UPD target ip and port number (the ip address is actually handled automatically in the emulator based on the address of the client which issues the `scan` command, however a dummy ip address must still be specified)|

## TODO 

- [ ] Sending data over TCP
- [ ] Saving / loading configurations from files
- [ ] Implement the same logic employed by Scanivale to calculate the scan and output rate based on the values set by `SET RATE`



