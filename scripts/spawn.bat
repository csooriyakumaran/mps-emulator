@echo off
start "MPS-01" .\bin\Release-windows-x86_64\mps-server\mps-server.exe --bind-ip 127.0.0.12 --port 23
start "MPS-02" .\bin\Release-windows-x86_64\mps-server\mps-server.exe --bind-ip 127.0.0.13 --port 23
start "MPS-03" .\bin\Release-windows-x86_64\mps-server\mps-server.exe --bind-ip 127.0.0.14 --port 23
start "MPS-04" .\bin\Release-windows-x86_64\mps-server\mps-server.exe --bind-ip 127.0.0.15 --port 23
start "MPS-05" .\bin\Release-windows-x86_64\mps-server\mps-server.exe --bind-ip 127.0.0.16 --port 23
start "MPS-06" .\bin\Release-windows-x86_64\mps-server\mps-server.exe --bind-ip 127.0.0.17 --port 23
start "MPS-07" .\bin\Release-windows-x86_64\mps-server\mps-server.exe --bind-ip 127.0.0.18 --port 23
start "MPS-08" .\bin\Release-windows-x86_64\mps-server\mps-server.exe --bind-ip 127.0.0.19 --port 23

