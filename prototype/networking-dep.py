import logging
import re
import socket
import socketserver
import sys
import threading
import queue
from typing import Tuple


    
class RequestHandler(socketserver.BaseRequestHandler):

    def __init__(self, request, client_address, server):
        self.logger = logging.getLogger('REQUEST HANDLER')
        self.logger.debug('initialize request handler')
        self.incoming_messages = queue.Queue(maxsize=100)
        socketserver.BaseRequestHandler.__init__(self, request, client_address, server)
        return

    def setup(self):
        self.logger.debug('setup request handler')
        return socketserver.BaseRequestHandler.setup(self)

    def handle(self):
        self.logger.debug('handle')
        # Echo the back to the client
        connected = True
        while connected:
            try:
                data = self.request.recv(4096)
            except ConnectionResetError as e:
                self.logger.error(e)
                return

            self.logger.debug('recv()->`%s`', data)
            self.incoming_messages.put(data)


        return

    def finish(self):
        self.logger.debug('finish')
        return socketserver.BaseRequestHandler.finish(self)

    def process_messages(self):
        while self.connected:
            data: bytes = self.incoming_messages.get()
            if 'disconnect' in data.decode('ASCII'):
                self.connected = False

            response = bytes("{}: {}\r\n>".format(threading.current_thread().name, data), 'ascii')
            self.request.send(response)


class TCPServer(socketserver.ThreadingTCPServer):

    def __init__(
            self,
            server_address: Tuple[str, int],
            handler_class: socketserver.BaseRequestHandler = socketserver.BaseRequestHandler
            ):

        self.logger = logging.getLogger('TCP SERVER')
        socketserver.TCPServer.__init__(self, server_address, handler_class)
        self.timouet = None
        return


    def serve_forever(self):
        self.logger.info('Handling requests, press <Ctrl-C> to quit')
        while True:
            self.handle_request()
        return

    def process_request(self, request, client_address):
        self.logger.debug('process_request(%s, %s)', request, client_address)
        return socketserver.TCPServer.process_request(self, request, client_address)

    def server_close(self):
        self.logger.debug('server_close')
        return socketserver.TCPServer.server_close(self)

    def finish_request(self, request, client_address):
        self.logger.debug('finish_request(%s, %s)', request, client_address)
        return socketserver.TCPServer.finish_request(self, request, client_address)

    def close_request(self, request_address):
        self.logger.debug('close_request(%s)', request_address)
        return socketserver.TCPServer.close_request(self, request_address)

class DataStreamHandler(socketserver.BaseRequestHandler):
    pass

class UDPServer(socketserver.UDPServer):
    def __init__(self, server_address, handler_class=RequestHandler):
        self.logger = logging.getLogger('UPD SERVER')
        self.logger.debug('__init__')
        socketserver.UDPServer.__init__(self, server_address, handler_class)
        return
