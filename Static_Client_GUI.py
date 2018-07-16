import os
import wx
from bluetooth import *
import time
import threading
import select
import atexit
# Class for setting up a connection with a server application
class BTServer:
    server_address = "60:02:92:A6:E1:E2"
    sock = None
    def __init__(self, name=None):
        self.name = name

    def connect(self):
        self.sock = BluetoothSocket(RFCOMM)
        self.sock.connect((self.server_address, 1))
        self.send("Hello")
        self.close()

    def receive(self):
        data = self.sock.recv(1024)
        print ("received [%s]" % data)
        return data

    def send(self, data):
        self.sock.send(data)

    def close(self): 
        if not self.sock is None:
            self.sock.close()
        
    
class Client_GUI(wx.Frame):
    device_uuid = "94f39d29-7d6d-437d-973b-fba39e49d4ee"
    device_selected = 0
    server = None
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

        # wx.ID_ABOUT and wx.ID_EXIT are standard ids provided by wxWidgets.
        #menuOpen = filemenu.Append(wx.ID_OPEN, "&Open", "Open a file")
        #filemenu.AppendSeparator()
        menuScan = filemenu.Append(wx.ID_ABOUT, "&Scan","Scan for servers")
        menuConnect = filemenu.Append(wx.ID_ANY, "&Connect", "Connect to selected server")
        menuExit = filemenu.Append(wx.ID_EXIT,"E&xit"," Terminate the program")

        # Creating the menubar.
        menuBar = wx.MenuBar()
        menuBar.Append(filemenu,"&File") # Adding the "filemenu" to the MenuBar
        self.SetMenuBar(menuBar)  # Adding the MenuBar to the Frame content.

        # Set events.
        self.Bind(wx.EVT_MENU, self.OnScan, menuScan)
        self.Bind(wx.EVT_MENU, self.OnConnect, menuConnect)
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
        # A message dialog box with an OK button. wx.OK is a standard ID in wxWidgets.
        thread = threading.Thread(target=self.BTConn)
        thread.start()
        #worker = BTDiscover(self, 0, [])
        #worker.start()
        #for addr in nearby_devices:
        #    print("  %s " % (addr))
        #dlg = wx.MessageDialog( self, "A small text editor", "About Sample Editor", wx.OK)
        #dlg.ShowModal() # Show it
        #dlg.Destroy() # finally destroy it when finished.

    def OnConnect(self, e):
        if (not self.lst.IsEmpty()):
            thread = threading.Thread(target=self.OpenSocket)
            thread.start()
        else:
            print("No selection")
    
    def OpenSocket(self):
        if not self.matches:
            pass
        else:
            self.connect(matches[self.lst.GetSelection()])
            sendThread = threading.Thread(target=self.SendPacket)
            thread.start()
    def SendPacket(self):
        threading.Timer(1, SendPacket).start()
        self.server.send("Test packet!")

    def BTScan(self):
        self.server = BTServer("BT_GUI")
        self.matches = self.server.find()
        # empty list
        if(not self.matches):
            wx.CallAfter(self.lst.Set, [])
        else:
            names = self.matches["name"]
            wx.CallAfter(self.lst.Set, names)
    def BTConn(self):
        self.server = BTServer("BT_GUI")
        self.server.connect()

    def OnExit(self,e):
        self.Close(True)  # Close the frame.
        exit()

    def exit(self):
        if not self.server is None:
            self.server.close()

app = wx.App(False)
frame = Client_GUI(None, "BT_Client")

atexit.register(frame.exit)
app.MainLoop()