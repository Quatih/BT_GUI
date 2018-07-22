import os
import wx
import bluetooth
from bluetooth import *
import time
import threading
import select
import atexit
import socket
device_uuid = "0fd5ca36-4e7d-4f99-82ec-2868262bd4e4"

#Class for setting up connection, receiving and sending to a BT device
class BTDevice:
    server_sock = None
    client_sock = None
    client_address = None
    connected = False
    def __init__(self, address=None, name=None):
        self.address = address
        self.name = name

    def connect(self):
        #self.port = get_available_port( RFCOMM )
        self.server_sock=  BluetoothSocket( RFCOMM )
        #self.server_sock.setblocking(False)
        self.server_sock.bind(("",PORT_ANY))
        self.server_sock.listen(1)
        advertise_service( self.server_sock, "BT_GUI", service_classes = [ SERIAL_PORT_CLASS ], profiles = [ SERIAL_PORT_PROFILE ] )
        
        self.client_sock,self.client_address = self.server_sock.accept()
        print ("Accepted connection from ",self.client_address)
        self.connected = True
        stop_advertising(self.server_sock)
        
    def receive(self):
        data = self.client_sock.recv(1024)
        print ("received [%s]" % data)
        return data

    def send(self, data):
        self.client_sock.send(data)

    def close(self): 
        if not self.server_sock is None:
            self.server_sock.close()
        if not self.client_sock is None:
            self.client_sock.close()        
    
class Server_GUI(wx.Frame):
    
    device_selected = 0
    connected_devices = []
    def __init__(self, parent, title):
        wx.Frame.__init__(self, parent, title=title, size=(300,150))
        #self.control = wx.TextCtrl(self, style=wx.TE_MULTILINE)
        panel = wx.Panel(self) 
        box = wx.BoxSizer(wx.HORIZONTAL) 
            
        self.text = wx.TextCtrl(panel,style = wx.TE_MULTILINE) 
            
        self.device_names = []   
        self.lst = wx.ListBox(panel, size = (100,-1), choices = self.device_names, style = wx.LB_SINGLE)
            
        box.Add(self.lst,0,wx.EXPAND) 
        box.Add(self.text, 1, wx.EXPAND) 
            
        panel.SetSizer(box) 
        panel.Fit() 
            
        self.Centre() 
        self.Bind(wx.EVT_LISTBOX, self.onListBox, self.lst) 
        self.Show(True)  
            
        self.CreateStatusBar() # A StatusBar in the bottom of the window

        # Setting up the menu.
        filemenu= wx.Menu()

        menuScan = filemenu.Append(wx.ID_ABOUT, "&Advertise","Advertise this server")
        #menuConnect = filemenu.Append(wx.ID_ANY, "&Connect", "Connect to selected server")
        menuExit = filemenu.Append(wx.ID_EXIT,"E&xit"," Terminate the program")

        # Creating the menubar.
        menuBar = wx.MenuBar()
        menuBar.Append(filemenu,"&File") # Adding the "filemenu" to the MenuBar
        self.SetMenuBar(menuBar)  # Adding the MenuBar to the Frame content.

        # Set events.
        self.Bind(wx.EVT_MENU, self.OnScan, menuScan)
        #self.Bind(wx.EVT_MENU, self.OnConnect, menuConnect)
        self.Bind(wx.EVT_MENU, self.OnExit, menuExit)
        #self.Bind(wx.EVT_MENU, self.OnOpen, menuOpen)
        self.Show(True)

    def onListBox(self, e): 
        self.device_selected = self.lst.GetSelection()
        self.text.AppendText( "Current selection: "+e.GetEventObject().GetStringSelection()+ " index: %d\n" %(self.device_selected))
        

    def OnOpen(self,e):
        """ Open a file"""
        self.dirname = ''
        dlg = wx.FileDialog(self, "Choose a file", self.dirname, "", "*.*", wx.FD_OPEN)
        if dlg.ShowModal() == wx.ID_OK:
            self.filename = dlg.GetFilename()
            self.dirname = dlg.GetDirectory()
            f = open(os.path.join(self.dirname, self.filename), 'r')
            self.control.SetValue(f.read())
            f.close()
        dlg.Destroy()

    def OnScan(self,e):
        print("Advertising")
        thread = threading.Thread(target=self.BTScan)
        thread.daemon = True
        thread.start()
    
    def OpenSocket(self):
        if not self.matches:
            pass
        else:
            self.connect(matches[self.lst.GetSelection()])
            sendThread = threading.Thread(target=self.ReceivePackets)
            sendThread.daemon = True
            thread.start()

    def ReceivePackets(self):
        threading.Timer(1, ReceivePackets).start()
        for dev in connected_devices:
            data = dev.receive()
            self.text.appendText(data + "\n")

    def BTScan(self):
        device = BTDevice("BT_GUI")
        device.connect()
        if (device.connected):
            self.connected_devices.append(device)

    def OnExit(self,e):
        self.Close(True)  # Close the frame.
        exit()
        
    def exit(self):
        if not self.connected_devices:
            pass
        else:
            for item in self.connected_devices:
                item.close()

app = wx.App(False)
frame = Server_GUI(None, "BT_Server")

atexit.register(frame.exit)
app.MainLoop()