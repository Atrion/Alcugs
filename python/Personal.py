#
# Alcugs Server Project
# $Id$
#
"""Each age.py is referenced by each game server age instance, this
is only a sample file, where I'm going to be adding some examples
code to explain how works the Alcugs API seen from Python.
Note, that not all is implemented, if I had more time, and/or more coders..."""

def onStartup():
    """onStartup is always run, when a server instance starts"""
    print "Personal.py init"

def onShutdown():
    """onShutdown is always run, when a server instance stops"""
    print "Personal.py stop"

def onIdle():
    pass

def onClientJoin(cli):
    print "onClientJoin " + str(cli)

def onClientLeft(cli):
    print "onClientLeft " + str(cli)

def onSystemMsg(msg):
    print "onSystemMsg"

def onPythonMsg(msg):
    print "onPythonMsg"

def onClientMsg(msg):
    print "onClientMsg"
    # The admin KI and user KI will be handled here.
    # And yes, you can have different admin/user ki commands
    # per each different age.

