import logging
import re
import socket
import sys
import queue
import signal
import struct
import threading
import time
import pickle
import numpy as np

from enum import Enum
from dataclasses import dataclass, asdict
from typing import Literal, Optional, Tuple, Dict, Callable
from types import FrameType
from networking import (
        TCPServer,
        UDPServer,
        )


HOST = "127.0.0.1"
PORT = 65432

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

class MPS4200:
    def __init__(self):
        self.scanning = False

class App:
    def __init__(self, host: str = HOST, port: int = PORT):
        self.host: str = host
        self.port: int = port
        self.running: bool = False
        self.scanning: bool = False
        self.logger = logging.getLogger('MPS EMULATOR')
        self.tcp: TCPServer = TCPServer(self.host, self.port)
        self.udp: UDPServer = UDPServer(self.host, self.port)
        self.mps: MPS4200 = MPS4200()
        self.tcp_thread = threading.Thread(
                target = self.tcp.start,
                daemon = True
                )
        self.tcp_thread = threading.Thread(
                target = self.tcp.start,
                daemon = True
                )

        self.message_queue = queue.Queue()
        self.message_thread = threading.Thread(
                target = self._process_message_queue,
                daemon = True
                )

        signal.signal(signal.SIGINT, self._signal_handler)
        signal.signal(signal.SIGTERM, self._signal_handler)

    def _signal_handler(self, sig: int, frame: Optional[FrameType]) -> None:
        self.logger.error("Signal recieved, stopping application ..")
        self.stop()

    def run(self):
        self.logger.debug('---- Running -------------------------------------')
        self.running = True
        self.message_thread.start()
        self.tcp.callback = self.handle_tcp_data
        self.tcp_thread.start()
        self.udp.start()

        return

    def stop(self):
        self.logger.warning('shutting down app')
        self.running = False
        self.tcp.stop()
        self.udp.stop()

    def handle_tcp_data(self, data: bytes, addr: tuple)->None:
        message = data.decode('ascii')

        self.tcp.send_string(message, addr)
        self.message_queue.put((data, addr))

    def _process_message_queue(self):
        while self.running:
            try:
                data, addr = self.message_queue.get(timeout=1)
                
                message = data.decode('ASCII').lower()

                if 'disconnect' in message:
                    self.stop()

                elif 'status' in message:
                    #- replace with status
                    self.tcp.send_string(self.status(), addr)
                elif 'scan' in message:
                    threading.Thread(target=self.start_scan, args=(addr,), daemon=True).start()

                elif 'stop' in message:
                    self.scanning = False

                else:
                    self.tcp.send_string('INVALID COMMAND:' + message, addr)

                
                self.message_queue.task_done()
            except queue.Empty:
                continue

    def status(self):
        if not self.running:
            return 'STATUS: DISCONNECTED'
        if self.scanning:
            return 'STATUS: SCANNING'
        
        return 'STATUS: READY'

    def start_scan(self, addr):

        self.logger.debug('---- START SCAN -----')
        self.scanning = True
        with open('data\\mps-0865.dat', 'rb') as f:
            t0 = time.time_ns()
            i = 0
            while self.scanning:
                t1 = time.time_ns()

                dt = (t1 - t0)/1e9

                if dt < 1/10:
                    continue
                
                t0 = t1

                buff = f.read(348)
                if not buff:
                    self.scanning = False
                    break

                self.logger.debug('bytes sent %d at t=%.2f' % (len(buff), t1/1e9))
                self.udp.send_bytes(buff, (self.host, self.port+1))
                i += 1




    def start_scan_dep(self, addr):
        self.logger.debug('---- START SCAN -----')
        t0 = time.time_ns()
        i = 0
        while i < 600:
            t1 = time.time_ns()
            if (t1 - t0)/1e9 * (i+1) < 1/10:
                continue
            t = t1 - t0

            packet = BinaryPacket(
                    packtype      = 0x0A, 
                    size          = 348,
                    frame         = np.int32(i+1),
                    serial_number = np.int32(100),
                    frame_rate    = np.int32(10),
                    valve_status  = np.int32(0), 
                    unit_index    = np.int32(0),
                    unit_conversion = np.float32(1.0),
                    ptp_start_s   = np.uint32(0),
                    ptp_start_ns  = np.uint32(0),
                    trigger_us    = np.uint32(0),
                    temperature   = np.zeros(shape=(8,), dtype=np.float32),
                    pressure      = np.zeros(shape=(64,), dtype=np.float32),
                    frame_time_s  = np.uint32(t // 1e9),
                    frame_time_ns = np.uint32(t),
                    trigger_s     = np.uint32(0),
                    trigger_ns    = np.uint32(0),
                    )

            # bytes_to_send = struct.pack('>hi', *packet.__dict__.values())
            # TODO (chris): HOW THE FUCK DO YOU DO THIS
            bytes_to_send = pickle.dump(packet)
            self.logger.debug('%s' % bytes_to_send)
            self.udp.send_bytes(bytes_to_send, (self.host, 65433))
            i += 1


def main() -> int:

    logging.basicConfig(level=logging.DEBUG, format='%(name)s: %(message)s',)
    app = App()

    try:
        app.run()
    except KeyboardInterrupt:
        app.stop()

    return 0

if __name__ == '__main__':
    sys.exit(main())
