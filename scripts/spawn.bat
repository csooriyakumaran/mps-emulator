@echo off
start "MPS-01" mps-emulator.exe --bind-ip 127.0.0.12 --port 23 --type 4264
start "MPS-02" mps-emulator.exe --bind-ip 127.0.0.13 --port 23 --type 4264 
start "MPS-03" mps-emulator.exe --bind-ip 127.0.0.14 --port 23 --type 4264
start "MPS-04" mps-emulator.exe --bind-ip 127.0.0.15 --port 23 --type 4232
start "MPS-05" mps-emulator.exe --bind-ip 127.0.0.16 --port 23 --type 4232
start "MPS-06" mps-emulator.exe --bind-ip 127.0.0.17 --port 23 --type 4216
start "MPS-07" mps-emulator.exe --bind-ip 127.0.0.18 --port 23 --type 4216
start "MPS-08" mps-emulator.exe --bind-ip 127.0.0.19 --port 23 --type 4216

