# UruVision
#
# Copyright 2004 Cyan Worlds, Inc.
#
# Please see the LICENSE File included with the distribution


#import UruHttp
import urllib2
import Tkinter, Canvas, Tix
import xml.parsers.expat
import sys, string, math, copy
import traceback
import ConfigParser
from traceback import print_exc
(kInvalid, kAgent, kLobby, kGame, kLookup, kPlayer, kInRoutePlayer) = range(0,7)
# konstants
kWindowSize = 600
kWindowCenter = kWindowSize/2
kRadiusInterval = 70
kNodeSize = 5
kDegreesToRadians = math.pi/180
kSmallishNumber = 1



# These get filled-out with member variables in systemview's xml parsing phase
class ServerInfo:
    pass
class AgeLink:
    pass
class AgeInfo:
    pass
    


# Node class
# All attributes starting with the 'x' char will be copied in copyFrom()
class Node:
    DiscToNode = {}
    kColors = {	kInvalid:('red','gray'),
                kAgent:	('black','purple'),
                kLobby:	('black','cyan'),
                kGame:	('black','yellow'),
                kLookup:('black','pink'),
                kPlayer:('black','green'),
                kInRoutePlayer:('black','gray')	}

    def __init__(self, canvas, type):
        self.fNew = 1
        self.fInit = 0
        self.xType = type
        self.xOwner = None	# in sysview data
        self.xParent = None	# in graph
        self.xChildren = []	# in graph
        # position
        self.fActualRadius = self.fDesiredRadius = 0.0
        self.fActualAngle = self.fDesiredAngle = 0.0
        self.fStartingRadius = 0.0
        self.fStartingAngle = 0.0
        # speed
        self.fRadiusDelta = 0.0
        self.fAngleDelta = 0.0 # degrees
        # sector
        self.fSectorSize = 0.0 # degrees
        self.fSectorOffset = 0.0 # degrees
        # graph info for layout
        self.fSubTreeSize = 0
        self.fRingLevel = 0
        #
        self.fMarkedForDeletion = 0
        self.fCanvas = canvas
        self.fDisc = None
        self.fLineToOwner = None
        self.fLabel = None
        
    def __hash__(self): return id(self)
    
    def copyFrom(self,other):
        # copy attrs that start with an 'f' char
        for key,value in other.__dict__.items():
            if key[0]=='x':
#                print "copying %s=%s from %s to %s" % (key, value, other.__class__,self.__class__)
                setattr(self, key, value)
        outlineColor, fillColor = Node.kColors[self.xType]
        self.fDisc.config(outline=outlineColor, fill=fillColor)
        pass
        
    def init(self):
        if self.fInit: return
        self.fInit = 1
        print "init",self
        # line
        self.fLineToOwner = Canvas.Line(self.fCanvas,0,0,0,0,fill='gray30',width=1)
        # disc
        self.fDisc = Canvas.Oval(self.fCanvas,0,0,0,0)
        self.fDisc.config(width=1)
        outlineColor, fillColor = Node.kColors[self.xType]
        self.fDisc.config(outline=outlineColor, fill=fillColor)
        Node.DiscToNode[self.fDisc] = self
        outlineColor, fillColor = Node.kColors[self.xType]
        self.fDisc.config(outline=outlineColor, fill=fillColor)
        # label
        self.fLabel = Canvas.CanvasText(self.fCanvas, 0,0, font=('helvetica', 8), justify='center', anchor='s', text='')
        
    def fini(self):
        if not self.fInit: return
        self.fInit = 0
#        print "fini",self
        self.deleteObjects()
        
    def deleteObjects(self):
        if self.fDisc:
            del Node.DiscToNode[self.fDisc]
            self.fCanvas.delete(self.fLineToOwner)
            self.fCanvas.delete(self.fLabel)
            self.fCanvas.delete(self.fDisc)
#            self.fLineToOwner.delete()
#            self.fLabel.delete()
#            self.fDisc.delete()
        self.fLineToOwner = None
        self.fLabel = None
        self.fDisc = None

    def getSpeed(self,start,desired,actual):
        dist = start-desired
        if dist==0:
            return 0
        distTraveled = start-actual
        x = (distTraveled/dist)*math.pi-(math.pi/2)
        y = 1/(1+x*x)
        return y
        
    def addChild(self,childNode):
#		print "added %s to %s" % (childNode,self)
        childNode.fParent = self
        for node in self.fChildren:
            if node.isEqualTo(childNode):
                node = childNode
                return
        self.fChildren.append(childNode)
        
    def removeChild(self,childNode):
#		print "removed %s from %s" % (childNode,self)
        childNode.fParent = None
        self.fChildren.remove(childNode)
        
    def reverseParentRelationshipRecurse(self):
        parent = self.fParent
        if parent:
            parent.reverseParentRelationshipRecurse()
            parent.removeChild(self)
            self.addChild(parent)
        
    def calcSubTreeSizeRecurse(self):
        self.fSubTreeSize = len(self.fChildren)
        for child in self.fChildren:
            child.calcSubTreeSizeRecurse()
            self.fSubTreeSize += child.fSubTreeSize
            
    def updateStartingCoordsRecurse(self):
        self.fStartingRadius = self.fActualRadius
        self.fStartingAngle = self.fActualAngle
        for child in self.fChildren:
            child.updateStartingCoordsRecurse()		

    def setRingLevel(self,level):
        self.fRingLevel = math.fabs(level)
        
    def setRingLevelRecurse(self,level=0):
        self.setRingLevel(level)
        for child in self.fChildren:
            child.setRingLevelRecurse(level+1)
            
    def layoutSubTreeRecurse(self):
    		try:
			sectorOffset = self.fSectorOffset
			# where we want the disc to be eventually
			self.fDesiredRadius = self.fRingLevel*kRadiusInterval
			self.fDesiredAngle = self.fSectorSize/2+self.fSectorOffset
			
			if self.fNew:
				self.fNew = 0
				if self.xOwner:
					self.fActualRadius = self.fStartingRadius = self.xOwner.fActualRadius
					self.fActualAngle = self.fStartingAngle = self.xOwner.fActualAngle
				else:
					self.fActualRadius = self.fStartingRadius = self.fDesiredRadius
					self.fActualAngle = self.fStartingAngle = self.fDesiredAngle
				
			for child in self.fChildren:
				child.fSectorSize = self.fSectorSize*(float(child.fSubTreeSize+1)/self.fSubTreeSize)
				if child.fSectorSize>180 and len(self.fChildren)>1:
					child.fSectorSize=180
				child.fSectorOffset = sectorOffset
				sectorOffset += child.fSectorSize
				child.layoutSubTreeRecurse()
		except:
			print_exc()
			            
    def animateSubTreeRecurse(self, center):
    		try:
			diffRadius = self.fActualRadius-self.fDesiredRadius
			if diffRadius<0: sign=-1
			else: sign=1
			radiusSpeed = 50*self.getSpeed(self.fStartingRadius,self.fDesiredRadius,self.fActualRadius)
			radiusSpeed = min(abs(diffRadius),radiusSpeed)
			if abs(diffRadius)>kSmallishNumber:
	#			self.fActualRadius -= diffRadius/3.0
				self.fActualRadius -= (radiusSpeed)*sign
			else:
				self.fActualRadius = self.fStartingRadius = self.fDesiredRadius
	
			diffAngle = self.fActualAngle-self.fDesiredAngle
			if diffAngle<0: sign=-1
			else: sign=1
			angleSpeed = 50*self.getSpeed(self.fStartingAngle,self.fDesiredAngle,self.fActualAngle)
			angleSpeed = min(abs(diffAngle),angleSpeed)
			if abs(diffAngle)>kSmallishNumber:
	#			self.fActualAngle -= diffAngle/3.0
				self.fActualAngle -= (angleSpeed)*sign
			else:
				self.fActualAngle = self.fStartingAngle = self.fDesiredAngle
			
			for child in self.fChildren:
				child.animateSubTreeRecurse(center)
			
			x1,y1 = fromPolar(self.fActualRadius, self.fActualAngle, center)
			self.fDisc.coords([(x1-kNodeSize,y1-kNodeSize),(x1+kNodeSize,y1+kNodeSize)])
			self.fLabel.config(text=self.getLabel())
			self.fLabel.coords([(x1-kNodeSize,y1-kNodeSize)])
			if self.xOwner and not isinstance(self.xOwner, SystemView):
				x1,y1 = fromPolar(self.fActualRadius, self.fActualAngle, center)
				x2,y2 = fromPolar(self.xOwner.fActualRadius, self.xOwner.fActualAngle, center)
				self.fLineToOwner.coords([(x1,y1),(x2,y2)])
			else:
				self.fLineToOwner.coords([(0,0),(0,0)])
		except:
			print_exc()

class PlayerNode(Node):
    def __init__(self,canvas,type):
        Node.__init__(self,canvas,type)
        self.fAcctName = ''
        self.fRealName =''
        self.fEmail = ''
        
    def dump(self):
#        print "%s guid=%s,nChildren=%d,subTreeSz=%d,parent=" % (self,self.xServerInfo.fGuid,len(self.fChildren),self.fSubTreeSize), self.fParent
#        print "\tring=%d,sectorSize=%f,sectorOffset=%f,Drad=%f,Dang=%f,Arad=%f,Aang=%f" % (self.fRingLevel,self.fSectorSize,self.fSectorOffset,self.fDesiredRadius,self.fDesiredAngle,self.fActualRadius,self.fActualAngle)
        pass
        
    def getLabel(self):
        try:
            if (self.xType == kPlayer):
                return "%s" % (self.xPlayerName)
            else:
                return "%s (%s)" % (self.xPlayerName, self.xState)
#            return "%s" % (self.xParent.xAgeLink.xAgeInfo.xAgeUserDefinedName)
#            return "%s" % (self.xName)
        except:
            return "Error"
            
    def getSummary(self):
        try:
            return "AcctName: %s\nAcctUUID: %s\nPlayerIdx: %s\nPlayerState: %s" % (self.xAcctName, self.xAccountUUID, self.xPlayerID, self.xState)
            #self.fEmail = gUserInfo.GetEmail(self.xAccountID)
            #self.fRealName = gUserInfo.GetRealName(self.xAccountID)
            #self.fAcctName = gUserInfo.GetAcctName(self.xAccountID)
            #return "RealName: %s\nAcctName: %s\nEMail: %s\nAcctIdx: %s\nPlayerIdx: %s" % (self.fRealName, self.fAcctName, self.fEmail, self.xAccountID, self.xPlayerID)
        except:
            return "Error"
        
    def getRClick(self):
        return getSummary()

    def isEqualTo(self,other):
        if isinstance(other,PlayerNode):
            return self.xPlayerID==other.xPlayerID
        return 0


class ServerNode(Node):
    def __init__(self,canvas,type):
        Node.__init__(self,canvas,type)
        
    def dump(self):
#        print "%s guid=%s,nChildren=%d,subTreeSz=%d,parent=" % (self,self.xServerInfo.xGuid,len(self.fChildren),self.fSubTreeSize), self.fParent
#        print "\tring=%d,sectorSize=%f,sectorOffset=%f,Drad=%f,Dang=%f,Arad=%f,Aang=%f" % (self.fRingLevel,self.fSectorSize,self.fSectorOffset,self.fDesiredRadius,self.fDesiredAngle,self.fActualRadius,self.fActualAngle)
        pass
        
    def getSummary(self):
        try:
            return "Addr: %s:%s\nGuid: %s\nName: %s" % (self.xServerInfo.xAddr,self.xServerInfo.xPort,self.xServerInfo.xGuid,self.xServerInfo.xName)
        except:
            return "Error"
        
    def isEqualTo(self,other):
        if isinstance(other,ServerNode):
            return self.xServerInfo.xGuid==other.xServerInfo.xGuid
        return 0
        

class LookupNode(ServerNode):
    def __init__(self,canvas):
        ServerNode.__init__(self,canvas,kLookup)
        
    def getLabel(self):
        return "Lookup"


class AgentNode(ServerNode):
    def __init__(self,canvas):
        ServerNode.__init__(self,canvas,kAgent)

    def getLabel(self):
        return "Agent:%s" % (self.xServerInfo.xAddr)

class LobbyNode(ServerNode):
    def __init__(self,canvas):
        ServerNode.__init__(self,canvas,kLobby)

    def getLabel(self):
        return "Lobby"

class GameNode(ServerNode):
    def __init__(self,canvas):
        ServerNode.__init__(self,canvas,kGame)

    def getLabel(self):
        try:
            return "%s" % (self.xAgeLink.xAgeInfo.xAgeInstanceName)
        except:
            return "Error"

    def getSummary(self):
        try:
            return "%s\nInstanceName: %s\n" % (ServerNode.getSummary(self),self.xAgeLink.xAgeInfo.xAgeInstanceName)
        except:
            return "Error"

class LookupServer:
    def __init__(self,canvas,addr,port,uri):
        self.fCanvas = canvas
        self.fAddr = addr
        self.fPort = port
        self.fUri = uri
    
    def getSystemView(self):
        systemView = None
        try:
            response = urllib2.urlopen('http://%s:%d%s' % (self.fAddr, self.fPort, self.fUri))
            data = response.read()
            #conn = UruHttp.HTTPConnection("%s:%s" % (self.fAddr, self.fPort))
            #conn.request("GET",self.fUri)
            #resp = conn.getresponse()
            #data = resp.read()
            systemView = SystemView(self.fCanvas)
            systemView.parse(data)
            #conn.sock.shutdown(1)
            #conn.sock.close()
        except:
            print_exc()
        #if conn:
        #    conn.close()

        return systemView

class UserInfo:
    fServer = ''
    fUru = ''
    fPort = ''
    ParserCurElm = {}
    ParserUI = None
    ParserCur = ''
    newWin = None
    AttemptedConns = 0 
    Data = {}
    def __init__(self,server,port,uri):
        self.fServer = server
        self.fUri    =  uri
        self.fPort   =  port
    def GetData(self):
        return
        # currently disabled
        #self.AttemptedConns += 1
        #try:
            #conn = UruHttp.HTTPConnection("%s:%s" % (self.fServer, self.fPort))
            #conn.request("GET",self.fUri)
            #resp = conn.getresponse()
            #data = resp.read()
            #conn.close()
            #gUserInfo.ParserUI = self
            #gUserInfo.ParserCurElm = {}
            #parser = xml.parsers.expat.ParserCreate()
            #parser.StartElementHandler = UIparserStartElement
            #parser.EndElementHandler = UIparserEndElement
            #parser.CharacterDataHandler = UIparserCharData
            #parser.Parse(data)
            #gUserInfo.ParserUI = None
            #gUserInfo.ParserCurElm = {}
        #except:
            #print_exc()

    def GetAcctName(self,AuthID):
        try:
            return gUserInfo.Data[AuthID]['acctname']
        except:
            return "Error Getting AcctName"   
    def GetRealName(self,AuthID):
        try:
            return "%s %s" % (gUserInfo.Data[AuthID]['firstname'], gUserInfo.Data[AuthID]['lastname'])
        except:
            return "Error Getting RealNamee"   
    def GetEmail(self,AuthID):
        try:
            return gUserInfo.Data[AuthID]['email']
        except:
            return "Error Getting EMail"   


def UIparserStartElement(name,attrs):
#    print "start element ", name
    if name=='user':
        try:
            gUserInfo.Data[gUserInfo.ParserCurElm['authidx']] = gUserInfo.ParserCurElm
        except:
            pass
    #		print_exc()
        gUserInfo.ParserCur = ''
        gUserInfo.ParserCurElm = {}
    if name=='acctname':
        gUserInfo.ParserCur = name;
        gUserInfo.ParserCurElm[name] = ''
    elif name=='group':
        gUserInfo.ParserCur = name;
        gUserInfo.ParserCurElm[name] = ''
    elif name=='cipacsidx':
        gUserInfo.ParserCur = name;
        gUserInfo.ParserCurElm[name] = ''
    elif name=='authidx':
        gUserInfo.ParserCur = name;
        gUserInfo.ParserCurElm[name] = ''
    elif name=='username':
        gUserInfo.ParserCur = name;
        gUserInfo.ParserCurElm[name] = ''
    elif name=='email':
        gUserInfo.ParserCur = name;
        gUserInfo.ParserCurElm[name] = ''
    elif name=='firstname':
        gUserInfo.ParserCur = name;
        gUserInfo.ParserCurElm[name] = ''
    elif name=='lastname':
        gUserInfo.ParserCur = name;
        gUserInfo.ParserCurElm[name] = ''

def UIparserEndElement(name):
#	print "end element ", name
    pass
def UIparserCharData(mydata):
    mydata = string.strip(mydata)
    if len(mydata)>0:
#		print 'charData', data
        gUserInfo.ParserCurElm[ gUserInfo.ParserCur ] = mydata

playerType = kInvalid

def parserStartElement(name,attrs):
    global playerType
    SystemView.ParserCurrElem = name
#	print "start element ", name
    if name=='SystemView':
        SystemView.ParserElementStack.append(SystemView.ParserSystemView)
    elif name=='Lookup':
        SystemView.ParserElementStack.append(LookupNode(SystemView.ParserSystemView.fCanvas))
    elif name=='Agent':
        SystemView.ParserElementStack.append(AgentNode(SystemView.ParserSystemView.fCanvas))
    elif name=='Lobby':
        SystemView.ParserElementStack.append(LobbyNode(SystemView.ParserSystemView.fCanvas))
    elif name=='Game':
        SystemView.ParserElementStack.append(GameNode(SystemView.ParserSystemView.fCanvas))
    elif name=='Player':
        SystemView.ParserElementStack.append(PlayerNode(SystemView.ParserSystemView.fCanvas,playerType))
    elif name=='Players':
        playerType = kPlayer
    elif name=='InRoutePlayers':
        playerType = kInRoutePlayer
    else:
        elem = SystemView.ParserElementStack.pop()
        SystemView.ParserElementStack.append(elem)
        try:
            cl = eval(name)
            clInst = cl()
            setattr(elem, 'x%s' % name, clInst)
            SystemView.ParserElementStack.append(clInst)
#            print clInst
        except:
            setattr(elem, 'x%s' % name, "")
            SystemView.ParserElementStack.append(elem)
    
def parserEndElement(name):
    global playerType
#	print "end element ", name
    if name=='Lookup' or name=='Agent' or name=='Lobby' or name=='Game' or name=='Player':
        elem = SystemView.ParserElementStack.pop()
        SystemView.ParserSystemView.fAllNodes.append(elem)
        if len(SystemView.ParserElementStack)>0:
            owner = SystemView.ParserElementStack.pop()
            SystemView.ParserElementStack.append(owner)
            elem.xOwner = owner
    elif name=='Players' or name=='InRoutePlayers':
        playerType = kInvalid
    else:
        elem = SystemView.ParserElementStack.pop()
    

def parserCharData(data):
    data = string.strip(data)
    if len(data)>0:
#		print 'charData', data
        elem = SystemView.ParserElementStack.pop()
        SystemView.ParserElementStack.append(elem)
        attr = "x%s" % SystemView.ParserCurrElem
        setattr(elem, attr, data)
        # print "set %s=%s on %s" % (attr, data, elem.__class__)

class SystemView:
    ParserSystemView = None
    ParserElementStack = []
    ParserCurrElem = ''
    MyCenter = kWindowCenter
    
    def __init__(self,canvas):
        self.fCanvas = canvas
        self.fCenterNode = None
        self.fAllNodes = []
        
    def parse(self,data):
        self.fAllNodes = []
        SystemView.ParserSystemView = self
        SystemView.ParserElementStack = []
        parser = xml.parsers.expat.ParserCreate()
        parser.StartElementHandler = parserStartElement
        parser.EndElementHandler = parserEndElement
        parser.CharacterDataHandler = parserCharData
        parser.Parse(data)
        SystemView.ParserSystemView = None
        SystemView.ParserElementStack = []
        self.fAllNodes.reverse()
        
    def erase(self):
        for node in self.fAllNodes:
            node.fini()
        self.fCenterNode = None
        self.fAllNodes = []
        
    def merge(self,other):
        for localNode in self.fAllNodes:
            localNode.fMarkedForDeletion = 1
            
        for otherNode in other.fAllNodes:
            localNode = self.findNodeLocally(otherNode)
            if localNode:
                localNode.copyFrom(otherNode)
                localNode.fMarkedForDeletion = 0
                localNode.init()
            else:
                self.fAllNodes.append(otherNode)
                    
        for localNode in self.fAllNodes[:]:
            if localNode.fMarkedForDeletion:
                if self.fCenterNode==localNode:
                    self.fCenterNode = localNode.xOwner
                self.fAllNodes.remove(localNode)
                localNode.fini()
                
        for localNode in self.fAllNodes:
            localOwnerNode = self.findNodeLocally(localNode.xOwner)
            localNode.xOwner = localOwnerNode
        
        self.buildGraph()
        
    def recenterOn(self,node):
        localNode = self.findNodeLocally(node)
        if localNode:
            self.fCenterNode = localNode
            self.buildGraph()
            self.layout()
        
    def buildGraph(self):
#		print "building graph"
        self.buildInitialGraph()
        self.buildRecenteredGraph()
        self.calcGraphInfo()
        
    def buildInitialGraph(self):
        for node in self.fAllNodes:
            node.fChildren = []
            node.fParent = None
            node.init()
        for node in self.fAllNodes:
            ownerNode = node.xOwner
            if ownerNode:
                ownerNode.addChild(node)
            
    def buildRecenteredGraph(self):
        self.initCenterNode()
        if self.fCenterNode:
            self.fCenterNode.reverseParentRelationshipRecurse()
        
    def calcGraphInfo(self):
        if self.fCenterNode:
            self.fCenterNode.setRingLevelRecurse()
            self.fCenterNode.calcSubTreeSizeRecurse()
            
    def initCenterNode(self):
        if self.fCenterNode:
            node = self.findNodeLocally(self.fCenterNode)
            if node:
                self.fCenterNode = node
        else:
            for node in self.fAllNodes:
                if isinstance(node,LookupNode):
                    self.fCenterNode = node
#		print "CenterNode:", self.fCenterNode


    def findNodeLocally(self,otherNode):	
        if otherNode==None:
            return None
        for localNode in self.fAllNodes:
            if localNode.isEqualTo(otherNode):
                return localNode
        return None

    def dump(self):
        for node in self.fAllNodes:
            node.dump()
    
    # set desired position
    def layout(self):
        if not self.fCenterNode:
            return
        self.fCenterNode.fSectorOffset = 0
        self.fCenterNode.fSectorSize = 360
        self.fCenterNode.layoutSubTreeRecurse()
        
    # set actual position
    def animate(self):
        global gUserInfo, kWindow, kWindowCenter
        if not self.fCenterNode:
            return
        self.fCenterNode.animateSubTreeRecurse(self.MyCenter)
        
    def ToolTipSwitch(self, node, x, y):
        #global gWindow, gUserInfo
        #print "Coords: %s" % (node.fLabel.coords)
        try:
            info = node.getRClick()
            UserInfo.newWin = Tkinter.Toplevel()
            UserInfo.newWin.title('%s - UruVision' % node.xName)
            UserInfo.newWin.wm_geometry('+%s+%s' % ((x),(y+10)))
            myText = Tkinter.Label(UserInfo.newWin,text="%s" % (info), justify=Tkinter.LEFT).pack()
            UserInfo.newWin.update()
            UserInfo.newWin.focus()
            #Canvas.CanvasText(self.fCanvas, 5, 5, info)
        except:
            print_exc()			

try:
    config = ConfigParser.ConfigParser()
    config.read("UruVision.ini")
    gUIHostname = ""#config.get("AuthXML","hostname")
    gUIPort     = 0#config.get("AuthXML","port")
    gUIUri      = ""#config.get("AuthXML","uri")
except:
    print_exc()

gUserInfo = UserInfo(gUIHostname, gUIPort, gUIUri)

def fromPolar(radius, angle, center):
    return center + radius * math.cos(angle * kDegreesToRadians), center + radius * math.sin(angle * kDegreesToRadians)
