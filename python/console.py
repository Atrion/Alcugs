#
# Alcugs Server Project
# $Id$
#
"""Interactive telnet console code"""
from alcugs import *

import threading
import pytelnetd
import sys

#You can ignore this code, since it does not work.
#(sometimes works, and sometines not, huh!?)
def startconsole():
    """Starts a telnetd service on the specified port with a python interpreter"""
    try:
        print "Spawning telnet server thread!!!"
        #sys.setcheckinterval(1)
        t=threading.Thread(target=pytelnetd.remoteInteractive, args=(1123,None,1,1))
        t.setDaemon(1)
        t.start()
        print "Spawned!"
    except Exception, detail:
        print "Cannot start telnet server, reason: %s" % (detail)
