import os
from bluetooth import *
import time
from PyQt5.QtBluetooth import *
from PyQt5.QtCore import *


# Class for setting up a connection with a server application
class BTServer:
   
    packets = 0
    sock = None
    connected = False
    discoverer = None

    def __init__(self):
        self.local_device = QBluetoothLocalDevice()
        if self.local_device.isValid():
            self.discoverer = QBluetoothServiceDiscoveryAgent()
            # connect signals to functions
            self.discoverer.finished.connect(self.scan_finished)
            self.discoverer.canceled.connect(self.scan_finished)
            self.discoverer.error.connect(self.scan_finished)
            self.discoverer.serviceDiscovered.connect(self.service_found)
        else:
            print("No valid bluetooth adapter")
    def connect(self, match):
        self.port = match["port"]
        self.name = match["name"]
        self.host = match["host"]
        print ("connecting to", self.host)
        self.sock=BluetoothSocket( RFCOMM )
        # for L2CAP 
        # self.sock.connect(self.host, self.port)
        # for RFCOMM
        self.sock.connect((self.host, self.port))
        packets = 0
    
    def service_found(self, info):
        
        print("Service found: %s" %(info.serviceName()))

    def scan_finished(self):
        print("Discovery finished")

    # returns list of servers with the matching service
    def find(self, _name = None, _uuid = None):
        
        
        
        if self.discoverer.isActive():
            self.discoverer.stop()
        else:
            print("Starting discovery")
        uuid = QBluetoothUuid(_uuid)
        # print(uuid.toString())
        # self.discoverer.setUuidFilter(uuid)

        self.discoverer.start(QBluetoothServiceDiscoveryAgent.FullDiscovery)
            # service_matches = find_service(uuid = _uuid)
            # if len(service_matches) == 0:
            #     print ("couldn't find the service")
            #     return [] 
            # else:
            #     print ("Found service!")
            #     for items in service_matches:
            #         print("%s" % items)
            #     return service_matches
        return []
        # except:
        #     print("No service found.")
        #     return []        

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

if __name__ == "__main__":
    server = BTServer()
    uuid = "0fd5ca36-4e7d-4f99-82ec-2868262bd4e4"
    server.find(_uuid = uuid, _name = "BT_Sense")