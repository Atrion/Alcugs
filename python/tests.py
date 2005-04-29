#
# Alcugs Server Project
# $Id$
#
"""Alcugs Python testing suite"""

from alcugs import *

def basic():
    try:
        print "Start some testing code"
        f=file("out.txt","wa")
        f.write("This file has been created from tests.py %s,%s" % (PtGetAgeName(), PtGetAgeGUID()))
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
    except Exception, detail:
        print "Basic test failed, reason: %s" % (detail)

def config():
    try:
        print "Testing the ptConfig manager"
        cfg=ptConfig()
        print "Is it true that your server is running at port: %s?" % cfg.getkey("port")
        print "Also, I think that your shard name is: %s" % cfg.getkey("shard.name")
        old_name=cfg.getkey("shard.name")
        print "I can do more things, for example, we want to change your shard name to something different"
        cfg.setkey("Boa shard","shard.name")
        print "Now, your shard name is: %s" % cfg.getkey("shard.name")
        cfg.setkey(old_name,"shard.name")
        print "Let's restore the old name, just in case %s" % cfg.getkey("shard.name")
        print "Unitialitzed data <%s>" % cfg.getkey("this_key_does_not_exist")
        print "That's all folks!"
    except Exception, detail:
        print "Basic test failed, reason: %s" % (detail)
        
def sdl_stuff():
    try:
        if(PtGetAgeName()=="Personal"):
          print "Testing the PtGetAgeSDL() function and the ptSDL class"
          sdl=PtGetAgeSDL()
          print "The psnlLibraryDoorClosed tuple should be:"
          print sdl['psnlLibraryDoorClosed']
          print "The CurentPage tuple should be:"
          print sdl['CurrentPage']
          print "...if it is really like that, SDL stuff works! :D"
          print "now we're testing what happens if the ptSDL class doesn't find a matching key:"
          print sdl['this_key_does_not_exist']
          print "you won't see me!"
    except Exception, detail:
        print "exception while doing the SDL test, reason: %s" % (detail)

def doall():
    basic()
    config()
    #sdl_stuff() #uncomment this when testing the sdl stuff
    
