- Datetime: 2026-03-21 19:17:27
- Model:    openai/gpt-5.1

> sorry, minor correction. The 0/1 start stop semantics for the TCP binary server are in addition to the existing command control semantics from the port 23 tcp connection. So if the SCAN command is recieved there it needs to trigger the TCP binary scanner as well (which again sends the same data that is send to the UDP socket)


✓  Task Completed

Thanks for clarifying the behavior; that actually fits very naturally into the design I outlined before. Here’s how to refine it so that:
- SCAN on the ASCII/Telnet port starts the same binary scanner as UDP.
- 0/1 on the TCP binary port is an *additional* control path, equivalent to STOP/SCAN.
- The same data frame is sent over UDP and TCP concurrently when both are active.

---
## 1. Core idea: one shared scanner state, multiple control paths

To mirror the real device, think in terms of **one logical scanner** with a single “acquisition state”, controlled from two places:
- ASCII/Telnet port (23) via SCAN / STOP (and other text commands).
- TCP binary port (503) via integer 1 (start) / 0 (stop).

Streaming then becomes a simple function of this shared state:
- If **scan is running**, send data frames.
- Where to send depends on which outputs are enabled:
  - UDP output enabled by ENUDP and `udp_port` config.
  - TCP binary output enabled by “there is a connected binary client on port 503”.

So you end up with something like this in conceptual terms:
- `bool scan_running;`          // shared across both interfaces
- `bool udp_enabled;`          // from ENUDP command on port 23
- `bool binary_client_connected;`  // from port 503 accept/disconnect

At every generated frame:
- If `scan_running && udp_enabled` -> send frame via UDP.
- If `scan_running && binary_client_connected` -> send frame via TCP binary.

This automatically gives you:
- SCAN starts **both** UDP and TCP streaming (if both outputs are present/enabled).
- STOP (or binary 0) halts both.
- Binary 1/0 on port 503 acts as an extra way to toggle `scan_running`.

---
## 2. Where to put this in the current code

### 2.1 Shared scan state in `Mps`

Right now, each TCP client handled by `ServerLayer` owns its own `mps::Mps` instance. To emulate a single physical device more closely, you should:
- Treat **one** `Mps` as the source of truth for acquisition and streaming.
- Both:
  - The Telnet/ASCII server (port 23 equivalent).
  - The TCP binary server (port 503).
  - The UDP output.
  all talk to this same `Mps`.

Concretely:
- In `mps::Mps` (mps.h / mps.cpp) make sure you have clear `Start()` / `Shutdown()` or `StartScan()` / `StopScan()` entry points that wrap the scan thread.
- Introduce an internal `bool m_ScanRunning;` (or reuse what you already have), which is set/cleared only by these methods.

Then:
- The handler for SCAN/STOP on port 23 calls `mps::Mps::StartScan()` / `StopScan()`.
- The handler for 1/0 on port 503 also calls the same `StartScan()` / `StopScan()`.

This is the key: **all control paths funnel into the same methods** on `Mps`.

### 2.2 Streaming fan-out inside the scan loop

At the end of `Mps::ScanThreadFn()` where a binary frame is built:
- Keep the existing UDP call:
  - Check `m_cfg.enudp` (your ENUDP flag) and send via `m_Server->StreamData(...udp_port...)` if true.
- Add a TCP-binary send path:
  - Check a `m_BinaryClientConnected` flag and, if true, send the same `Buffer` over TCP using the binary server’s `SendBuffer(...)`.

You now have **one scan loop** and **two output sinks** fed from it, both controlled by the same `m_ScanRunning` state.

---
## 3. Implementing the TCP binary server with 0/1 semantics

### 3.1 Second TCP server (port 503)

Add a second TCP server alongside your existing `aero::networking::Server` used for ASCII control:
- Either:
  - A second `aero::networking::Server` instance configured with `port = 503`, or
  - A small bespoke TCP loop using `Socket` utilities.

Give it callbacks like:
- `OnBinaryClientConnected(id)`
- `OnBinaryClientDisconnected(id)`
- `OnBinaryDataReceived(id, Buffer buf)`

Behavior:
- On first connection: set `binary_client_connected = true` and remember the connection id/socket.
- If another client connects: either reject it or drop the previous one – the real device is single-user per interface.
- On disconnect: set `binary_client_connected = false` and (optionally) stop the scan.

### 3.2 Parsing 0/1 as additional scan control

In `OnBinaryDataReceived`:
- Interpret incoming bytes as either a 32-bit integer or simple byte values, depending on how you want to mirror the real protocol.
- When you read:
  - `1`: call `mps::Mps::StartScan()`.
  - `0`: call `mps::Mps::StopScan()`.

Because `Mps::StartScan()` / `StopScan()` are the same methods used by the SCAN/STOP command handlers on port 23, this naturally gives you:
- 0/1 on port 503 is **in addition** to SCAN/STOP; whichever is called last wins.
- SCAN (ASCII) will start scanning and thus cause frames to be sent to both UDP and TCP (if a binary client is connected), without needing an extra 1 on port 503.

---
## 4. ENUDP and port 23 behavior in this model

To align with the device behavior you described:
- `ENUDP` (on the ASCII port) continues to only affect **UDP** output:
  - It toggles `m_cfg.enudp`.
  - The scan loop checks `m_cfg.enudp` before sending via UDP.
- SCAN/STOP still live on the ASCII port and map to `StartScan()` / `StopScan()`.

Resulting behavior:
- If `ENUDP 1` and you issue SCAN: you get both UDP and TCP streaming (if a binary client is present).
- If `ENUDP 0` and you issue SCAN: you get **TCP-only** streaming (still sending frames to port 503 client).
- If you never send SCAN/STOP on ASCII, but you connect to port 503 and send `1`: scanning starts and frames go to:
  - UDP if `ENUDP 1`.
  - TCP if a binary client is connected.

This matches your “additive” 0/1 semantics on port 503 relative to the existing command/control channel.

---
## 5. Do we need an explicit enable for the TCP binary server?

To mirror the real device:
- **No explicit enable command is needed**.
- Simply:
  - Always listen on port 503.
  - Use 1/0 or SCAN/STOP to start/stop the scan.
  - Start sending frames as soon as `scan_running` is true and a binary client is connected.

You *may* add a command on port 23 to disable the binary TCP interface (for testing or safety), but it is not required by your current description of the hardware.

---
## 6. Final behavior summary

With this design:
- There is one logical scanner (`Mps`) with a single `scan_running` state.
- SCAN/STOP on the ASCII/Telnet port and 1/0 on the TCP binary port all map to `StartScan()` / `StopScan()` on that scanner.
- Every frame produced by the scan loop is:
  - Sent over UDP if ENUDP is on.
  - Sent over TCP binary if a client is connected to port 503.
- SCAN on port 23 automatically “triggers the TCP binary scanner” as you requested, since it starts the same underlying scan used by both outputs.

This gives you a faithful emulation of the real-world device’s ASCII control, UDP streaming, and TCP binary server, while keeping the implementation conceptually simple and consistent with your existing codebase.
