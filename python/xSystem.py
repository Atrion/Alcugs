#
# Alcugs Server Project
#
"""xSystem.py is referenced by all game server instances, this is for global code"""

from alcugs import *

import threading
import pytelnetd
import sys

#You can ignore this code, since it does not work.
def startconsole():
    """Starts a telnetd service on the specified port with a python interpreter, muhahahaha"""
    try:
        print "Spawning telnet server thread!!!"
        #sys.setcheckinterval(1)
        t=threading.Thread(target=pytelnetd.remoteInteractive, args=(1123,None,1,1))
        t.setDaemon(1)
        t.start()
        print "Spawned!"
    except Exception, detail:
        print "Cannot start telnet server, reason: %s" % (detail)


def onStartup():
    """onStartup is always run, when a server instance starts"""
    print "Hello xSystem.py called by ", PtGetAgeName(), " instance ", PtGetAgeGUID()
# Uncoment the next line to avoid running some useless testing code.
#    return
    print "Start some teting code"
    f=file("out.txt","wa")
    f.write("This file has been created from xSystem.py %s,%s" % (PtGetAgeName(), PtGetAgeGUID()))
    f.close()
    f=ptLog("test.log")
    f.write("This file has been generated from the xSystem.py")
    f.write("works, or SEGFAULTS?");
    f.flush()
    f.write("hmmm, I'm %s,%s, can you see me?\n" % (PtGetAgeName(), PtGetAgeGUID()))
    f.close()
    a=PtGetPythonLog()
    a.write("Hello, now I'm writting into the python log")
    a.close()
    a.write("Close don't works on the python log file")
    f.write("But it works on other files, with the issue, that this text goes nowhere")
    x=ptLog("AlcugsRocks.log")
    x.write("This file has been generated from the xSystem.py")
    x.write("Yeah, can you do this in the Plasma servers?")
    x.close()
    del f
    del x,a
#Uncoment the next line to start an interactive console on port 1123.
    #startconsole()

def onShutdown():
    """onShutdown is always run, when a server instance stops"""
    print "Bye, the server is stopping me :(..."


def onIdle():
    return
    print "onIdle Current net time is: %i,%i" % (PtGetNetTime(),PtGetNetMicros())
    print "But current time is: %i,%i" % (PtGetTime(),PtGetMicros())
    a=2+3
    b=5+9
    c=a+b
    print c
    pass

