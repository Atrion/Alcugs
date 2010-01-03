# UruVision
#
# Copyright 2004 Cyan Worlds, Inc.
#
# Please see the LICENSE File included with the distribution

import string, socket, sys, os, time, math
import Tkinter, Canvas, Tix
import tkFileDialog
import tkMessageBox
import UruServers
import ConfigParser

# now used to thread off our HTTP connections
import thread

from UruServers import Node, SystemView, LookupServer, UserInfo
from UruServers import kWindowSize, kWindowCenter, kRadiusInterval, kNodeSize, gUserInfo
from UruServers import kInvalid,kAgent, kLobby, kGame, kLookup, kPlayer, kInRoutePlayer
import traceback
from traceback import print_exc
import gc

#gc.set_debug(gc.DEBUG_LEAK)


config = ConfigParser.ConfigParser()
config.read("UruVision.ini")
if len(sys.argv)<2:
	print "Alternatively, you can pass the LookupServer hostname and URI via the command line."
	try:
		gLookupHostName = config.get("LookupServer","hostname")
		gLookupServerPort = config.get("LookupServer","port")
		gLookupServerUri = config.get("LookupServer","uri")
		print "Using LookupServer from UruVision.ini"
	except:
		print "Could Not Read UruVision.ini for LookupServer Hostname. Pass it via the command line!"
		time.sleep(15)
		sys.exit(1)
else:
	gLookupHostName = sys.argv[1]
	if len(sys.argv) > 3:
		gLookupServerPort = sys.argv[2]
		gLookupServerUri = sys.argv[3]
	else:
		gLookupServerPort = 80
		gLookupServerUri = sys.argv[2]

print "LookupServer Hostname:",gLookupHostName," Port: ",gLookupServerPort," URI: ",gLookupServerUri

# Konstants
kNumCircles = kWindowSize/kRadiusInterval
kPollInterval = 5000
kUpdateInterval = 50

# Create window object
gWindow = Tkinter.Tk()
gWindow.withdraw()
gWindow.title('UruVision - %s' % gLookupHostName)
x = (gWindow.winfo_screenwidth() / 2) - (kWindowSize/2)
y = (gWindow.winfo_screenheight() / 2) - (kWindowSize/2)
#print kWindowSize
gWindow.wm_geometry('%sx%s+%s+%s' % (kWindowSize,kWindowSize,x,y))

# Now enabled by default, the resize code..
gEnableResize = 1
# Create our menu
FILE_TYPES=[("PostScript File",".ps"),("All files","*")] 
def tkMakeBorderLess():
	global gSystemView, gCanvas, gUserInfo, gWindow
	gWindow.overrideredirect(1)
def tkMakeBorderOn():
	global gSystemView, gCanvas, gUserInfo, gWindow
	gWindow.overrideredirect(0)
def tkEnableResize():
	global gEnableResize
	gEnableResize = 1
def tkDisableResize():
	global gEnableResize
	gEnableResize = 0
def tkSaveCanvas():
	data = gCanvas.postscript()
	FileName = tkFileDialog.asksaveasfilename(defaultextension=".ps", filetypes=FILE_TYPES)
	#Pressed cancel.. or no file name?
	if FileName == "" or FileName == None:
		pass
	else:
		tkSaveToFile(FileName, data)
	return
def tkExit():
	sys.exit(0)
def tkView_menu():
	view_btn = Tkinter.Menubutton(gMenu, text='View', underline=0)
	view_btn.pack(side=Tkinter.LEFT, padx="2m")
	view_btn.menu = Tkinter.Menu(view_btn)
	view_btn.menu.add_command(label="Border On", underline=0, command=tkMakeBorderOn)
	view_btn.menu.add_command(label="Border Off", underline=0, command=tkMakeBorderLess)
	view_btn.menu.add_command(label="Resize On", underline=0, command=tkEnableResize)
	view_btn.menu.add_command(label="Resize Off", underline=0, command=tkDisableResize)
	view_btn['menu'] = view_btn.menu
	return view_btn
def tkFile_menu():
	file_btn = Tkinter.Menubutton(gMenu, text='File', underline=0)
	file_btn.pack(side=Tkinter.LEFT, padx="2m")
	file_btn.menu = Tkinter.Menu(file_btn)
	file_btn.menu.add_command(label="Save", underline=0, command=tkSaveCanvas)
	file_btn.menu.add_command(label="Exit", underline=0, command=tkExit)
	file_btn['menu'] = file_btn.menu
	return file_btn
def tkSaveToFile(FileName, data):
	try:
		File=open(FileName,"w")
		File.write(data)
		File.close()
	except IOError:
		print_exc()
		tkMessageBox.showerror("Save error...",
		"Could not save to '%s'"%FileName) 
	return

gMenu = Tkinter.Frame(gWindow)
gMenu.pack(fill=Tkinter.X, side=Tkinter.TOP)
gMenu.tk_menuBar(tkFile_menu(), tkView_menu())

# Create canvas object
gCanvas = Tkinter.Canvas(gWindow, width=kWindowSize, height=kWindowSize, bg='white')
gCanvas.pack()


### HoverWindow class
class HoverWindow:
	def __init__(self,canvas):
		self.fCanvas = canvas
		self.fHighlight = None
		self.fLabel = None
		self.fNode = None
		
	def init(self):
		self.fHighlight = Canvas.Oval(self.fCanvas,0,0,0,0)
		self.fHighlight.config(outline="#FF0000",width="1")
		self.fLabel = Canvas.CanvasText(self.fCanvas, 7,7, font=('courier', 8), justify='left', anchor='nw', text='')
		
	def fini(self):
		if self.fHighlight: self.fCanvas.delete(self.fHighlight)
		if self.fLabel: self.fCanvas.delete(self.fLabel)
		self.fHighlight = None
		self.fLabel = None
		self.fNode = None
		
	def updateInfo(self,node):
		if not node:
			self.fini()
			return
		if not self.fHighlight:
			self.init()
		self.fNode = node
		self.fLabel.config(text=self.fNode.getSummary())
		
	def update(self):
		if not self.fNode: return
		try:
			coords = self.fNode.fDisc.coords()
			self.fHighlight.coords([(coords[0]-2,coords[1]-2),(coords[2]+2,coords[3]+2)])
		except:
			# this happens when fNode is deleted from system view out from under us.
			self.fini()



gLookupServerAddr = socket.gethostbyname(gLookupHostName)
gLookupServer = LookupServer(gCanvas, gLookupServerAddr, gLookupServerPort, gLookupServerUri)
gSystemView = SystemView(gCanvas)
gHoverWindow = HoverWindow(gCanvas)


# Create rings
gRings = []
def CreateRings():
	global kRadiusInterval, kWindowCenter, kNumCircles, gRings, gCanvas
	gRings = []
	for ri in range(kNumCircles):
		r = kRadiusInterval * ri
		h = 0xA0+kNumCircles*ri
		if(h>255):
			h = 255
		c = gCanvas.create_oval(kWindowCenter-r, kWindowCenter-r,
			kWindowCenter+r, kWindowCenter+r,
#			outline='#300210')
			outline='#bb%02x%02x' % (h,h))
		gCanvas.lower(c)
		gRings.append(c)
CreateRings()
def DeleteRings():
	global kRadiusInterval, kWindowCenter, kNumCircles, gRings, gCanvas
	for Ring in gRings:
		gCanvas.delete(Ring)

# Periodically poll for system view
def pollSystemView():
	global gLookupServer, gSystemView, gCanvas
	t = ()
	thread.start_new_thread(realPollSystemView, t)
	gCanvas.tk.createtimerhandler(kPollInterval, pollSystemView)
def realPollSystemView():
#	print "polling system view"
	global gLookupServer, gSystemView, gCanvas
	sysView = gLookupServer.getSystemView()
	if sysView:
		gSystemView.merge(sysView)
#        gSystemView.dump()
		gSystemView.layout()
    
# Periodically update the window
def updateWindow():
	global gSystemView, gCanvas, gUserInfo
	gSystemView.animate()
	gHoverWindow.update();
	gCanvas.update()
	gCanvas.tk.createtimerhandler(kUpdateInterval, updateWindow)

# Respond to mouse left-clicks
def leftClick(event):
	global gSystemView, gUserInfo
	ids = gCanvas.find_overlapping(event.x-2, event.y-2, event.x+2, event.y+2)
	for id in ids:
		node = Node.DiscToNode.get(gCanvas.items.get(id))
		if node:
			gSystemView.fCenterNode.updateStartingCoordsRecurse()
			gSystemView.recenterOn(node)
			break
		
def rightClick(event):
	global gSystemView, gUserInfo, gWindow
	try:
		UserInfo.newWin.destroy()
	except:
		pass
	ids = gCanvas.find_overlapping(event.x-2, event.y-2, event.x+2, event.y+2)
	for id in ids:
		node = Node.DiscToNode.get(gCanvas.items.get(id))
		if node:
			try:
				if node.fType == kPlayer or node.fType == kInRoutePlayer:
					# to find the *real* location of the event on the 
					# entire screen we need to find where the root window is located
					x = gWindow.winfo_rootx()
					y = gWindow.winfo_rooty()
					gSystemView.ToolTipSwitch(node, event.x + x, event.y + y+20)
					break
			except:
				print_exc()

def windowReSize(event):
	global gCanvas, kWindowSize, kWindowCenter, kNumCircles, kRadiusInterval, gEnableResize, gSystemView
	if gEnableResize:
		oldkWindowSize = kWindowSize;
		if event.height > event.width:
			kWindowSize = event.width
		else:
			kWindowSize = event.height
		if (abs(kWindowSize - oldkWindowSize) > 10 and abs(kWindowSize - oldkWindowSize) < 500):
			#print "Orig: %s New: %s Diff: %s" % (oldkWindowSize,kWindowSize,abs(kWindowSize - oldkWindowSize))
			kWindowCenter = kWindowSize/2
			#print "Center: %s" % (kWindowCenter )
			kNumCircles = kWindowSize/kRadiusInterval
			gCanvas.configure(width=kWindowSize)
			gCanvas.configure(height=kWindowSize)

			#redraw our rings....
			DeleteRings()
			CreateRings()

			gSystemView.MyCenter = kWindowCenter

		else:
			kWindowSize = oldkWindowSize

	return
    
def updateHoverWindowInfo(event):
	global gSystemView, gUserInfo
	ids = gCanvas.find_overlapping(event.x-2, event.y-2, event.x+2, event.y+2)
	for id in ids:
		node = Node.DiscToNode.get(gCanvas.items.get(id))
		if node:
			gHoverWindow.updateInfo(node)
			break
	pass
    

# Bind mouse and keyboard events
gCanvas.bind("<Button-1>", leftClick, '')
gCanvas.bind("<Button-3>", rightClick, '')
gWindow.bind("<Configure>", windowReSize)
gWindow.bind("<Any-Motion>", updateHoverWindowInfo)

#gWindow.overrideredirect(1)
# Setup initial poll for system view
gCanvas.tk.createtimerhandler(1, pollSystemView)
gCanvas.tk.createtimerhandler(1, updateWindow)

config = ConfigParser.ConfigParser()
config.read("UruVision.ini")
gUIHostname = ""#config.get("AuthXML","hostname")
gUIPort     = 0#config.get("AuthXML","port")
gUIUri      = ""#config.get("AuthXML","uri")
gUserInfo = UserInfo(gUIHostname, gUIPort, gUIUri)


def loadUserUT():
	splash = Tkinter.Toplevel()
	x = (gWindow.winfo_screenwidth() / 2) - (400/2)
	y = (gWindow.winfo_screenheight() / 2) - (150/2)
	splash.wm_geometry('400x150+%s+%s' % (x,y))
	splash.overrideredirect(1)
	text = Tkinter.Label(splash,text="\n\n\nLoading User Information....\n\n\n", justify=Tkinter.CENTER).pack()
	splash.update()
	splash.focus
	gUserInfo.GetData()
	splash.destroy()
	gWindow.deiconify()
	gWindow.focus()
    

### Run
loadUserUT()
try:
	Tkinter.mainloop()
except:
	print_exc()	
