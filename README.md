# MPS-EMULATOR

An emulated MPS-42xx pressure scanner.

The emulator starts a TCP server on the default port 23 that accepts connections from a single client, and handles commands following the [Scanivalve MPS Protocol](https://github.com/csooriyakumaran/scanivalve-mps-protocol) as defined in the [Scanivavle Hardware, Software, and User Manual](https://scanivalve.com/wp-content/uploads/2026/03/MPS4200_v401_260304.pdf).

## USAGE
```bash
 mps-emulator.exe [<OPTIONS>] [<ARGUMENTS>]
```
| OPTIONS              | ARGUMENTS             | DESCRIPTION                                      |
| -------------------- | --------------------- | ------------------------------------------------ |
| `-h`, `--help`       |                       | Print help message and exit.                     |  
| `-v`, `--version`    |                       | Print software version and exit.                 |
| `--enable-console`   |                       | Enable the local input console.                  |
| `-t`, `--type`       |`<scanner-type>`       | MPS type. Allowed: 4216, 4232, 4264 (default)    |
| `-p`, `--port`       |`<port-number>`        | listening port for the server. (default 23)      |
| `--bind-ip`          |`<ip-address>`         | bound ip for the server. (default 127.0.0.1)     |

e.g.:
```bash
 mps-emulator.exe --type 4216 --bind-ip 127.0.0.101 --port 23 
```

Once running, connect to the server as any other scanner. 

Launch many instances of this application, each with unique IP addresses in the local host domain (i.e., 127.0.0.0/8) to simulate connecting to more than one device.

### Controlling to the Emulator

```bash

# starts server with vitural device type MPS-4264 on ip 127.0.0.12:23
mps-emulator.exe --type 4264 --bind-ip 127.0.0.12 --port 23

```

Connections made over TCP/Telnet on port 23 will be granted full command/control

```bash
telnet 127.0.0.12 23
> LIST S
SET RATE 10
SET FPS 0
SET UNITS PA 6894.76
SET FORMAT T A, F B, B B
SET TRIG 0
SET ENFTP 0
> 

```

### Receiving Data

### UDP

UDP data transfer can be enabled using the `ENUDP` command as well as setting the UDP target destinations with the `SET IPUDP <IP> <PORT>` command. This corresponds to the IP and port of the listening UDP client.

```bash
> LIST UDP
SET ENUDP 0
SET IPUDP 127.0.0.1 23
> SET ENUDP 1
SET ENDUP 1
```

Data format is controlled using the `SET FORMAT F` command. The only acceptable format is `B`

```bash
> SET FORMAT F B
SET FORMAT F B
```


### TCP Binary Server

Like the physical devices, the virtual device emulates the TCP Binary Server on port 503. Data sent to this destination will follow the format set by:

```bash
> SET FORMAT B B
SET FORMAT B B
```

Or for the Labview binary data format:

```bash
> SET FORMAT B L
SET FORMAT B L
```

Connections to this socket can issue basic start/stop scanning commands by sending the integer 0 or 1. 

### FTP

NOT IMPLEMENTED

### Local Console

If enabled with the `--enable-console` flag, a local console runs on a separate listening thread that waits for user input through stdin. The user can send TCP messages to any connected client simply by typing them in the console and hitting enter. 

For testing, scanner commands can be issued directly from the server console by prefixing the normal MPS commands with a `/` (e.g. `/set fps 300`), although in the debug configuration build, the number of logging statements that are printed makes this somewhat annoying. Commands issued via the console will have their output echoed to all connected clients. Scan requests made from the console will result in data being streamed to all listening clients.

Because the console waits for user input, it will hang on reboot/shutdown until user input is written to `stdin`. Consequently, when not testing with the console it is better to not set the `--enable-console` flag so that the server can be remotely restarted with the `reboot` or `restart` command, or stopped with the `shutdown` command. 

## Data Generation

Scan data are generated pseudo-randomly following a normal distribution with a fixed standard deviation. At the beginning of each scan, the random number generator is seeded with a hash of the device type and serial number (which is generated from the IP address of the server). Consequently, every scan from the same virtual device instance produces the same sequence of numbers. The virtual device produces random samples as if it were a 24bit ADC with a physical range of +/- 1 psi. The standard deviation of the randomly generated data corresponds to 0.05% of the full scale range.

> [!IMPORTANT] 
> The repeatability of the rng sequence may be platform/compiler dependant, so may not be guaranteed. 

### Command Formatting

Commands can be sent to the server from clients (or the console if prefixed by `/`) following the same format as described in the MPS manual. That is, commands must be less than 79 characters, and must terminate with \<CR\>\<LF\> (i.e., ascii-13 ascii-10). This implementation requires all upper case or all lower case.

e.g.:
```console
> SET RATE 100\r\n
```
will send the command `SET RATE` with the arguments `100` setting the sampling rate to 100 Hz.

To maintain compatibility with the ScanTel terminal emulator, which uses the TelNet protocol, the server accepts commands that are issued one character at a time (including backspaces to remove characters) until a carriage return is sent. 

### Commands and Syntax

Below are listed the commands which must be sent as shown and temrinated with `<CR><LF>`. Argument parameters are shown inside `< >` and optional parameters are enclosed in `[ ]`.  The following exceptions to this are special characters:

- `<BS>`: Backspace ( `0x08` )
- `<TAB>`: Horizontal tab ( `0x09` )
- `<LF>`: Line feed ( `0x0A` )
- `<CR>`: Carriage return ( `0x0D` )
- `<ESC>`: Escape ( `0x1B` )

Equivalent or alias commands are separated by `|`.

Consult the MPS manual for more detailed descriptions of each command, including examples. 

> [!IMPORTANT] 
> Not all commands are implemented (e.g., no FTP server is emulated)

***GENERAL COMMANDS***

| SYNTAX                              | DESCRIPTION                                                                                                 |
| ----------------------------------- | ----------------------------------------------------------------------------------------------------------- |
| `VER`                               | Return the version                                                                                          |
| `REBOOT` \| `RESTART`               | Restart the server after closing all connections                                                            |
| `SHUTDOWN`                          | Shutdown the server                                                                                         |
| `STOP` \| `<ESC>` (TelNet only)     | Stop a scan if one is started                                                                               |
| `STATUS`                            | Returns the status. e.g.:<br> - `STATUS: READY`<br> - `STATUS: SCAN`<br> - `STATUS: CALZ`                   |
| `CALZ`                              | Start a zero-cal (in the emulator this simply sets the status to `STATUS: CALZ` for 2 seconds)              | 
| `VALVESTATE`                        | Returns the valve state (always `PX` for the emulator) ***NOT IMPLEMENTED***                                |
| `TRIG` \| `<TAB>`                   | Returns one frame of data (requires  `SET TRIG 1`) ***NOT IMPLEMENTED***                                    |
| `TREAD [<MODULE>]`                  | Returns the (simulated) module temperature on all or requested module (from 1 to 8). ***NOT IMPLEMENTED***  |
| `SAVE [<cfg>]`                      | Save the configuration (not implemented)                                                                    |
| `SCAN`                              | Start a scan                                                                                                |

***LIST COMMANDS***

| SYNTAX                              | DESCRIPTION                           |
| ----------------------------------- | ------------------------------------- |
| `LIST S`                            | Returns the scan settings             |
| `LIST UDP`                          | Returns the UDP settings              |
| `LIST M`                            | Returns the misc. settings            |

***SET COMMANDS***

| SYNTAX                              | DESCRIPTION                           |
| ----------------------------------- | ------------------------------------- |
| `SET RATE <RATE>`                   | Set the sampling rate in Hz of the scanner. If `RATE` is less than 500, the scanner averages internally.|
| `SET ENUDP <OPT>`                   | `0`: Disable UDP<br>`1`: Enable UDP   |
| `SET IPUDP <IP> <PORT>`             | Set the UPD target ip and port number |
| `SET SIM <VALUE>`                   | Set the Simulation variable in decimal or hex. <BR>`SET SIM 0x40` (i.e., 64) Simulate 64 Ch. Device with padded legacy format<BR> `SET SIM 0x04` **NOT IMPLEMENTED**|

***TCP BINARY COMMANDS***

The emulator accepts basic commands on TCP port 503

| SYNTAX                              | DESCRIPTION                           |
| ----------------------------------- | ------------------------------------- |
| `1`                                 | (e.g., the integer 1) Start a scan    |
| `0`                                 | (e.g., the integer 0) Stop a scan     |


## Building from Source

***REQUIRES***

1. [git](https://git-scm.com/)
2. [CMake](https://cmake.org/) (Version 3.14 or higher)
3. C++20 compiler
    - ***Windows***:
      - `MSVC` from [Visual Studio 2022 (v17)](https://visualstudio.microsoft.com/downloads/) with c++20 support or,
      - `clang/LLVM` with c++20 support 
    - ***Linux/macOS*** (Not tested):
      - `GCC 11+` or,
      - `Clang 13+` 
4. A build tool / generator backend
    - For `MSVC` workflows:
        - [Visual Studio 2022 (v17)](https://visualstudio.microsoft.com/downloads/) (generator `-G "Visual Studio 17 2022"`)
    - For `LLVM` workflows:
        - [Ninja]() (generator `-G "Ninja"`)
5. [Windows SDK](https://learn.microsoft.com/en-us/windows/apps/windows-sdk/downloads) /  Win32 libraries (on Windows)
    - Required for:
        - WinSock (`ws2_32.lib`)
        - Windows resource compilation (`resources.rc`) for icon and version info
    - These are typically installed automatically with Visual Studio and its C++ / Windows Desktop workloads.


***CLONE REPO***
```bash
git clone https://github.com/csooriyakumaran/mps-emulator.git
cd mps-emulator
```

#### MSVC Multi-configuration
From the project root:

```bash
# configure build files in directoy `build`
cmake -S . -B build -G "Visual Studio 17 2022" -A x64

# compile the targets
cmake --build build --config Release
```

#### Ninja + Clang
Ninja is a single configuration build, so the project must be configured separately for debug/release builds. Alternatively, specify separate build directories for each configuration (as shown below)

```bash
# Debug
cmake -S . -B build-debug -G "Ninja" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_CONFIG=Debug
cmake --build build-debug

# Release
cmake -S . -B build-release -G "Ninja" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_CONFIG=Release
cmake --build build-release
```


#### Installing / Packaging

After configuring and building, the application can be packaged for release or installed locally

***PACKAGING***
```bash
# package for release, e.g., 
cmake --install build --config Release --prefix stage/v0.2.0
```
This creates the following directory structure:
```text
./stage/
|   v0.2.0/
|   |   bin/
|   |   |   mps-eumulator.exe
|   |   LICENSE
|   |   README.md
```


Which can be zipped, and released on [GitHub](https://github.com) (using web UI or [GitHub CLI](https://cli.github.com/) as below).

```bash
gh release create <TAG> --title <TITLE> --generate-notes ./releases/<release>.zip
#e.g.
gh release create v0.2.0 --title "v0.2.0" --generate-notes ./releases/v0.2.0.zip
```

> [!IMPORTANT]
> - tag must already exist, if not create one
> ```bash 
> git tag -a v0.2.0 -m "Release v0.2.0"
> ```
> - staged release directory must be zipped and copied to ./releases/ directory

***LOCAL BINARY-ONLY INSTALL***
```bash
cmake --install build --prefix <path> --component Runtime
```

This adds the executable to `<path>/bin/mps-emulator.exe`. 

#### Cleaning

To clean the built binaries:
```bash
cmake --build <build-dir> --target clean
```

For a complete cleanup, simply delete the build directory:
```bash
rmdir /s /q <build-dir>
```


## TODO 

- [x] Sending data over TCP
- [x] Implement the same logic employed by Scanivale to calculate the scan and output rate based on the values set by `SET RATE`
- [ ] Saving / loading configurations from files

