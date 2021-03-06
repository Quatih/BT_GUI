import os

# Requires pybluez library
import bluetooth as bt
#from bluetooth import *
import time
# Class for setting up a connection with a server application
class BTServer:
    packets = 0
    sock = None
    connected = False
    def __init__(self):
        pass
    def connect(self, match):
        self.port = match["port"]
        self.name = match["name"]
        self.host = match["host"]
        print ("connecting to", self.host)
        self.sock=bt.BluetoothSocket( bt.RFCOMM )
        # for L2CAP 
        # self.sock.connect(self.host, self.port)
        # for RFCOMM
        self.sock.connect((self.host, self.port))

    # returns list of servers with the matching service
    def find(self, _name = None, _uuid = None):
        try:
            service_matches = bt.find_service(uuid = _uuid)
            if len(service_matches) == 0:
                print ("couldn't find the service")
                return [] 
            else:
                print ("Found service!")
                for items in service_matches:
                    print("%s" % items)
                return service_matches
        except:
            print("No service found.")
            return []        

    def receive(self, bytes):
        try:
            data = self.sock.recv(bytes)
            self.packets = self.packets + len(data)
            # print ("received [%s]" % data)
            return data
        except: 
            print("Receive failed")
            self.close()
            return []

    def send(self, data):
        try:
            self.sock.send(data)
        except:
            print("sending data failed")
            self.close()
            raise SendError()

    def close(self): 
        if not self.sock is None:
            self.sock.close()
            self.connected = False


class Error(Exception):
  pass

class ReceiveError(Error):
  def __init__(self):
    pass

class SendError(Error):
  def __init__(self):
    pass

