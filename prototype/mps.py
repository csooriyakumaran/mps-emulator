
import logging
import queue
import struct
import time
import sys

from dataclasses import dataclass

import numpy as np

@dataclass
class MPSConfig:
    fps: np.int32 = np.int32(600)
    rate: np.int32 = np.int32(10)
    output_rate: np.int32 = np.int32(1)
    tcp_ip:   str = '127.0.0.1'
    tcp_port: int = 65432
    udp_ip:   str = '127.0.0.1'
    udp_port: int = 65432
    unit_index: np.int32 = np.int32(23) # Pa

@dataclass
class BinaryPacket:
    packtype:        np.int32 # 0x0A for Raw/EU
    size:            np.int32 # 348
    frame:           np.int32 
    serial_number:   np.int32
    frame_rate:      np.float32 # Hz
    valve_status:    np.int32 # (0: Px, 1: Cal)
    unit_index:      np.int32
    unit_conversion: np.float32
    ptp_start_s:     np.uint32
    ptp_start_ns:    np.uint32
    trigger_us:      np.uint32
    temperature:     np.ndarray #[float32]
    pressure:        np.ndarray #[float32]
    frame_time_s:    np.uint32
    frame_time_ns:   np.uint32
    trigger_s:       np.uint32
    trigger_ns:      np.uint32

    def to_bytes(self) -> bytes:
        return struct.pack('>hi', self)

@dataclass
class MPS4200:
    config: MPSConfig
    tcp:    TCPServer
    udp:    UDPServer
    buffer: queue.Queue


    def scan(self):
        t0 = time.time_ns()
        for i in range(self.config.fps):
            t1 = time.time_ns()
            packet = BinaryPacket(
                    packtype      = 0x0A, 
                    size          = 348,
                    frame         = np.int32(i+1),
                    serial_number = self.config.serial_number,
                    frame_rate    = self.config.frame_rate,
                    valve_status  = np.int32(0), 
                    unit_index    = self.config.unit_index,
                    unit_conversion = np.float32(1.0),
                    ptp_start_s   = np.uint32(t0 // 1e9),
                    ptp_start_ns  = np.uint32(t0),
                    trigger_us    = np.uint32(t0 // 1e3),
                    temperature   = np.zeros(shape=(8,), dtype=np.float32),
                    pressure      = np.zeros(shape=(64,), dtype=np.float32),
                    frame_time_s  = np.uint32(t1 // 1e9),
                    frame_time_ns = np.uint32(t1),
                    trigger_s     = np.uint32(t0 // 1e0),
                    trigger_ns    = np.uint32(t0)
                    )
        self.buffer.put(packet)
