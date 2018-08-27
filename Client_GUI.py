import os
import wx
import time
import threading
import select
import atexit
import pickle
import PyQt5.QtBluetooth

from BTServer import *
from TimerThread import *
filename = "BT_GUI.dat"
outputfile = "output.csv"
sampling_rate = 20480
packet_length = 520
class Client_GUI(wx.Frame):
    device_uuid = "0fd5ca36-4e7d-4f99-82ec-2868262bd4e4"
    device_selected = 0
    server = None
    matches = None
    output_file = None
    sendThread = None
    recThread = None
    countThread = None
    fileAccess = False
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
        self.server = BTServer()
        try:
            fh = open(filename, "rb")
        except:
            print ("Error: can\'t open file")
            thread = threading.Thread(target=self.BTScan)
            thread.daemon = True
            thread.start()
        else:
            try:
                self.matches = pickle.load(fh)
            except:
                print("Failed to load from file")
                thread = threading.Thread(target=self.BTScan)
                thread.daemon = True
                thread.start()
            else:
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
            
            fh.close()

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
                self.countThread.stop_timer()
                print("Closed connection to %s" %self.server.name)
                self.exit()
            else:
                print("No server connected")
        else:
            print("No server connected")
    def OpenSocket(self):
        if not self.matches:
            pass
        else:
            try:
                self.server.connect(self.matches[self.lst.GetSelection()])
                self.server.connected = True
            except Exception as err:
                print("Failed to connect: ", err )
                self.server.connected = False
            else:
                self.output_file = open(outputfile, "a+")
                self.sendThread = TimerThread(1, 0.25, self.SendPacket)
                self.sendThread.daemon = True
                self.sendThread.start()
                self.sendThread.start_timer()
                self.recThread = TimerThread(0.0001, 0.000025, self.ReceivePacket)
                self.recThread.daemon = True
                self.recThread.start()
                self.recThread.start_timer()
                self.countThread = TimerThread(1, 0.25, self.countPackets)
                self.countThread.daemon = True
                self.countThread.start()
                self.countThread.start_timer()

    def countPackets(self):
        print("total packets", self.server.packets)
        self.server.packets = 0

    def SendPacket(self):
        #threading.Timer(1, self.SendPacket).start()
        self.server.send("Test packet!")

    def ReceivePacket(self):
        data = self.server.receive()
        if (data is not None):
            self.fileAccess = True
            # print(data)
            length = len(data) 
            print("Receive bytes:", length)
            # if the data returned includes multiple sequences, go over each one
            for packet_num in range(1, int(length / packet_length), 1):
                # get 64 bytes of unsigned usecs at the start
                time = [data[i+(packet_length*(packet_num-1))] for i in range(0,7,1)]
                time = long.from_bytes(time, byteorder = 'big', signed = False) 
                usec_step = 1e6/sampling_rate 
                for i in range(packet_num*packet_length, 9 + (packet_num-1)*packet_length, -2):
                    item = [data[i-1], data[i]]
                    # substract from 4096 because the data is inverted
                    num = 4096 - int.from_bytes(item, byteorder='big', signed=False)
                    try:
                        self.output_file.write("%u;%u\n" %(num, time))
                    except Exception as err: 
                        print("Failed to write to file:", err)  
                    print(num, time/(1e6)) 
                    time = time - usec_step
            self.fileAccess = False
        else:
            if not self.output_file is None:
                self.output_file.close()
            raise ReceiveError()

    def BTScan(self):
        print("Scanning for servers")
        self.matches = self.server.find(_name = "BT_Sense", _uuid = self.device_uuid) 
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
            try:
                fh = open(filename, "wb+")
            except:
                print ("Error: can\'t find file or create it")
            else:
                #try: 
                pickle.dump(self.matches, fh)
                #except:
                   # print("failed to dump object")
                #else:
                print("Wrote matches to file")
                fh.close()

            wx.CallAfter(self.lst.AppendItems, names)

    def OnExit(self,e):
        self.Close(True)  # Close the frame.
        exit()

    def exit(self):
        if not self.sendThread is None:
            self.sendThread.terminate()
        if not self.recThread is None:
            self.recThread.terminate()
        if not self.countThread is None:
            self.countThread.terminate()

        if not self.output_file is None:
            while(self.fileAccess):
                pass
            self.output_file.close()
        if not self.server is None:
            self.server.close()

app = wx.App(False)
frame = Client_GUI(None, "BT_Client")

# create an exit call when the program exits
atexit.register(frame.exit)
app.MainLoop()