#
# Alcugs Server Project
# $Id$
#
"""xSystem.py is referenced by all game server instances, this is for global code"""

from alcugs import *
import tests

def onStartup():
    """onStartup is always run, when a server instance starts"""
    print "Hello xSystem.py called by %s instance %s" % (PtGetAgeName(),PtGetAgeGUID())
    tests.doall()    
#Start an interactive console on port 1123.
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

