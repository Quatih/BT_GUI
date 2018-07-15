import os
import wx
import bluetooth
import time
import threading
import select

#Wrapper for setting up connection, receiving and sending to a BT device
class BTDevice:
    port = None
    server_sock = None
    client_sock = None
    client_address = None
    def __init__(self, address=None, name=None):
        self.address = address
        self.name = name

    def connect(self):
        self.port = get_available_port( RFCOMM )
        self.server_sock=BluetoothSocket( RFCOMM )
        #self.server_sock.setblocking(False)
        self.server_sock.bind(("",port))
        self.server_sock.listen(1)
        self.client_sock,self.client_address = self.server_sock.accept()
        print ("Accepted connection from ",address)
        
    def receive(self):
        data = client_sock.recv(1024)
        print ("received [%s]" % data)
        return data

    def send(self, data):
        self.client_sock.send(data)

    def close(self): 
        self.client_sock.close()
        self.server_sock.close() 
    
class MainWindow(wx.Frame):
    device_uuid = "94f39d29-7d6d-437d-973b-fba39e49d4ee"
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

        # wx.ID_ABOUT and wx.ID_EXIT are standard ids provided by wxWidgets.
        #menuOpen = filemenu.Append(wx.ID_OPEN, "&Open", "Open a file")
        #filemenu.AppendSeparator()
        menuAbout = filemenu.Append(wx.ID_ABOUT, "&About"," Information about this program")
        menuExit = filemenu.Append(wx.ID_EXIT,"E&xit"," Terminate the program")
        # Creating the menubar.
        menuBar = wx.MenuBar()
        menuBar.Append(filemenu,"&File") # Adding the "filemenu" to the MenuBar
        self.SetMenuBar(menuBar)  # Adding the MenuBar to the Frame content.

        # Set events.
        self.Bind(wx.EVT_MENU, self.OnAbout, menuAbout)
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

    def OnAbout(self,e):
        # A message dialog box with an OK button. wx.OK is a standard ID in wxWidgets.
        thread = threading.Thread(target=self.BTDisco)
        thread.start()
        #worker = BTDiscover(self, 0, [])
        #worker.start()
        #for addr in nearby_devices:
        #    print("  %s " % (addr))
        #dlg = wx.MessageDialog( self, "A small text editor", "About Sample Editor", wx.OK)
        #dlg.ShowModal() # Show it
        #dlg.Destroy() # finally destroy it when finished.

    def OnConnectButton(self, e):
        thread = threading.Thread(target=self.OpenSocket)
        thread.start()
    
    def OpenSocket(self):
        port = 1
        backlog = 100
        server_sock = BluetoothSocket(RFCOMM)
        server_sock.bind(self.addresses(self.device_selected),port)
        server_sock.listen(backlog)
        client_sock, client_info = server_sock.accept()
        print("accepted connection from %s" %(client_info))
        data = client_sock.recv(1024)
        print("received: %d" %(data))

        pass

    def BTDisco(self):
        try:
            #service_matches = find_service(name = None, uuid = device_uuid, address = None )
            nearby_devices = bluetooth.discover_devices(duration=8, flush_cache=True, lookup_class=False, lookup_names=True)
            self.addresses, self.names = zip(*nearby_devices)
        except: 
            print("No devices found")
            self.addresses = []
            self.names = []
            pass
        if len(self.addresses) > 0:
            print("Found %d devices" % (len(self.addresses)))
            for addr in self.addresses:
                print("id %s" % (addr))
            
            wx.CallAfter(self.lst.Set, self.names)

    def OnExit(self,e):
        self.Close(True)  # Close the frame.

app = wx.App(False)
frame = MainWindow(None, "BTD test")

app.MainLoop()