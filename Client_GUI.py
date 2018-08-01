import os
import wx
from bluetooth import *
import time
import threading
import select
import atexit
# custom thread with timer functions and a callback
class TimerThread(threading.Thread):
    def __init__(self, timeout=3, sleep_chunk=0.25, callback=None, *args):
        threading.Thread.__init__(self)

        self.timeout = timeout
        self.sleep_chunk = sleep_chunk
        if callback == None:
            self.callback = None
        else:
            self.callback = callback
        self.callback_args = args

        self.terminate_event = threading.Event()
        self.start_event = threading.Event()
        self.reset_event = threading.Event()
        self.count = self.timeout/self.sleep_chunk

    def run(self):
        while not self.terminate_event.is_set():
            while self.count > 0 and self.start_event.is_set():
                # print self.count
                # time.sleep(self.sleep_chunk)
                # if self.reset_event.is_set():
                if self.reset_event.wait(self.sleep_chunk):  # wait for a small chunk of timeout
                    self.reset_event.clear()
                    self.count = self.timeout/self.sleep_chunk  # reset
                self.count -= 1
            if self.count <= 0:
                self.start_event.set()
                #print 'timeout. calling function...'
                self.callback(*self.callback_args)
                self.count = self.timeout/self.sleep_chunk  #reset

    def start_timer(self):
        self.start_event.set()

    def stop_timer(self):
        self.start_event.clear()
        self.count = self.timeout / self.sleep_chunk  # reset

    def restart_timer(self):
        # reset only if timer is running. otherwise start timer afresh
        if self.start_event.is_set():
            self.reset_event.set()
        else:
            self.start_event.set()

    def terminate(self):
        self.terminate_event.set()

# Class for setting up a connection with a server application
class BTServer:
    device_uuid = "0fd5ca36-4e7d-4f99-82ec-2868262bd4e4"
    sock = None
    connected = False
    def __init__(self, port=None, name=None):
        self.name = name
        self.port = port

    def connect(self, match):
        self.port = match["port"]
        self.name = match["name"]
        self.host = match["host"]
        print ("connecting to ", self.host)
        self.sock=BluetoothSocket( RFCOMM )
        # for L2CAP 
        # self.sock.connect(self.host, self.port)
        # for RFCOMM
        self.sock.connect((self.host, self.port))
        self.connected = True
    # returns list of servers with the matching service
    def find(self, _uuid = None):
        try:
            service_matches = find_service(uuid = _uuid)
            if len(service_matches) == 0:
                print ("couldn't find the service", self.name)
                return [] 
            else:
                print ("Found service!")
                for items in service_matches:
                    print("%s" % items)
                return service_matches
        except:
            print("No service found.")
            return []        

    def receive(self):
        data = self.sock.recv(1024)
        print ("received [%s]" % data)
        return data

    def send(self, data):
        self.sock.send(data)

    def close(self): 
        if not self.sock is None:
            self.sock.close()
            self.connected = False
        
    
class Client_GUI(wx.Frame):
    
    device_selected = 0
    server = None
    matches = None
    def __init__(self, parent, title):
        wx.Frame.__init__(self, parent, title=title, size=(300,150))
        #self.control = wx.TextCtrl(self, style=wx.TE_MULTILINE)
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        panel = wx.Panel(self) 
        box = wx.BoxSizer(wx.HORIZONTAL) 
            
        self.text = wx.TextCtrl(panel,style = wx.TE_MULTILINE) 
            
        self.device_names = []   
        self.lst = wx.ListBox(panel, size = (200,-1), choices = self.device_names, style = wx.LB_SINGLE)
            
        box.Add(self.lst, 0, wx.EXPAND) 
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
        menuDisconnect = filemenu.Append(wx.ID_ANY, "&Disconnect", "Disconnect from connected server")
        menuExit = filemenu.Append(wx.ID_EXIT,"E&xit"," Terminate the program")

        # Creating the menubar.
        menuBar = wx.MenuBar()
        menuBar.Append(filemenu,"&File") # Adding the "filemenu" to the MenuBar
        self.SetMenuBar(menuBar)  # Adding the MenuBar to the Frame content.

        # Set events.
        self.Bind(wx.EVT_MENU, self.OnScan, menuScan)
        self.Bind(wx.EVT_MENU, self.OnConnect, menuConnect)
        self.Bind(wx.EVT_MENU, self.OnDisconnect, menuDisconnect)
        
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
        thread = threading.Thread(target=self.BTScan)
        thread.daemon = True
        thread.start()
        #worker = BTDiscover(self, 0, [])
        #worker.start()
        #for addr in nearby_devices:
        #    print("  %s " % (addr))
        #dlg = wx.MessageDialog( self, "A small text editor", "About Sample Editor", wx.OK)
        #dlg.ShowModal() # Show it
        #dlg.Destroy() # finally destroy it when finished.

    def OnConnect(self, e):
        if self.server.connected:
            print("Already connected")
        elif (not self.lst.IsEmpty()):
            thread = threading.Thread(target=self.OpenSocket)
            thread.daemon = True
            thread.start()
        else:
            print("No selection")
    def OnDisconnect(self, e):
        if not self.server is None:
            if self.server.connected:
                self.sendThread.stop_timer()
                self.recThread.stop_timer()
                self.exit()
                print("Closed connection to %s" %self.server.name)
            else:
                print("No server connected")
        else:
            print("No server connected")
    def OpenSocket(self):
        if not self.matches:
            pass
        else:
            self.server.connect(self.matches[self.lst.GetSelection()])
            #self.sendThread = threading.Thread(target=self.SendPacket)
            self.sendThread = TimerThread(1, 0.25, self.SendPacket)
            self.sendThread.daemon = True
            self.sendThread.start()
            self.sendThread.start_timer()
            self.recThread = TimerThread(1, 0.25, self.ReceivePacket)
            #self.recThread = threading.Thread(target=self.ReceivePacket)
            self.recThread.daemon = True
            self.recThread.start()
            self.recThread.start_timer()

    def SendPacket(self):
        #threading.Timer(1, self.SendPacket).start()
        self.server.send("Test packet!")

    def ReceivePacket(self):
        #threading.Timer(1, self.ReceivePacket).start()
        rec = self.server.receive()

    def BTScan(self):
        print("Scanning for servers")
        self.server = BTServer("BT_Sense")
        self.matches = self.server.find(_uuid = self.server.device_uuid) 
        # empty list
        if(not self.matches):
            wx.CallAfter(self.lst.Clear)
        else:
            wx.CallAfter(self.lst.Clear)
            names = []
            for service in self.matches:
                name = service["name"]
                if not name is None:
                    name = name.decode("utf-8")
                else:
                    name = "N/A"
                host = service["host"]
                if host is None:
                    host = "N/A"
                
                name += ", host: "
                name += host
                names.append(name)
            wx.CallAfter(self.lst.AppendItems, names)

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