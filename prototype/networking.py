import queue
import logging
import socket
import signal
import sys
import threading
from typing import Literal, Optional, Tuple, Dict, Callable
from types import FrameType

LOCALHOST = "127.0.0.1"
DEFAULTPORT = 65432


class Server:
    def __init__(
        self,
        host: str = LOCALHOST,
        port: int = DEFAULTPORT,
        protocol: Literal["TCP", "UDP"] = "TCP",
    ) -> None:
        self.host: str = host
        self.port: int = port
        self.protocol: Literal["TCP", "UDP"] = protocol.upper()
        self.server: socket.socket | None = None
        self.running: bool = False
        self.logger: logging.Logger = logging.getLogger("%s-SERVER" % self.protocol)
        self.send_queue = queue.Queue()
        self.lock = threading.Lock()
        self.send_thread = threading.Thread(
            target=self._process_and_send_queue, daemon=True
        )
        self.connections: Dict = {}
        self.callback: Callable[[bytes, Tuple[str, int]], None] | None = None

        signal.signal(signal.SIGINT, self._signal_handler)
        signal.signal(signal.SIGTERM, self._signal_handler)

    def _signal_handler(self, sig: int, frame: Optional[FrameType]) -> None:
        self.logger.error("Signal recieved, stopping server ..")
        self.stop()

    def send_bytes(self, data: bytes, addr: Tuple | None = None):
        """
        Queue data to be sent. For UDP, address must be provided.
        """

        self.send_queue.put((data, addr))

    def send_string(self, data: str, addr: Tuple | None = None):
        """
        Queue data to be sent. For UDP, address must be provided.
        """

        message = bytes('%s%s' % (data, '>'), 'ascii')
        self.send_queue.put((message, addr))

    def _process_and_send_queue(self):
        """
        Continously process and send the data from the queue.
        """
        while self.running:
            try:
                data, addr = self.send_queue.get(timeout=1)
                if self.protocol == "UDP":
                    self.logger.debug('sending %d bytes to %s' % (len(data), repr(addr)))
                    self.server.sendto(data, addr)
                elif self.protocol == "TCP":
                    if addr in self.connections:
                        conn = self.connections[addr]
                        conn.sendall(data)

                self.send_queue.task_done()

            except queue.Empty:
                continue

    def start(self):
        """
        Starts the server based on the protocol (TCP or UDP).
        """
        if self.running:
            self.logger.warning("Server is already running")
            return

        self.running = True
        self.server = socket.socket(
            socket.AF_INET,
            socket.SOCK_STREAM if self.protocol == "TCP" else socket.SOCK_DGRAM,
        )

        self.server.settimeout(1)
        self.server.bind((self.host, self.port))

        if self.protocol == "TCP":
            self.server.listen()
            self.logger.info(
                "Listening for connection on %s:%d" % (self.host, self.port)
            )
            self.send_thread.start()
            self._accept_connections()

        else:
            self.logger.info(
                "Listening for connection on %s:%d" % (self.host, self.port)
            )
            self.send_thread.start()
            self._handle_datagrams()

    def _accept_connections(self):
        """
        Handles incoming TCP connections.
        """
        while self.running:
            try:
                conn, addr = self.server.accept()
                self.logger.info("connection established from %s" % repr(addr))
                # could add a connection callback here
                self.connections[addr] = conn
                threading.Thread(
                    target=self._handle_client_connection, args=(conn, addr)
                ).start()
            except socket.timeout:
                continue

            except Exception as e:
                self.logger.error(e)
        self.logger.debug('stop accepting connections')

    def _handle_client_connection(self, conn, addr):
        """
        Handles data from a single TCP client connection.
        """
        with conn:
            while self.running:
                data = conn.recv(4096)
                if not data:
                    break
                self.logger.debug("data recieved %s" % data)
                if self.callback:
                    self.callback(data, addr)
                # self.send_data(data.decode("ASCII"), addr)

        self.logger.warning("connection lost with %s" % repr(addr))
        if addr in self.connections:
            del self.connections[addr]

    def _handle_datagrams(self):
        """
        Handles incoming UDP datagrams
        """
        while self.running:
            try:
                data, addr = self.server.recvfrom(1024)
                self.logger.debug("data recieved from %s: %s" % (addr, data))
            except socket.timeout:
                continue

    def stop(self):
        """
        Stops the Server
        """
        for addr, conn in self.connections.items():
            self.send_string('Closing Connection', addr)
        self.running = False
        if self.server:
            self.server.close()
            self.logger.info("Sever Stopped")
        # self.send_thread.join()


class TCPServer(Server):
    def __init__(self, host=LOCALHOST, port=DEFAULTPORT):
        super().__init__(host, port, "TCP")


class UDPServer(Server):
    def __init__(self, host=LOCALHOST, port=DEFAULTPORT):
        super().__init__(host, port, "UDP")


if __name__ == "__main__":
    logging.basicConfig(
        level=logging.DEBUG,
        format="%(name)s: %(message)s",
    )
    server = TCPServer()
    try:
        server.callback = server.send_string
        server.start()
    except KeyboardInterrupt:
        server.stop()
        sys.exit()
