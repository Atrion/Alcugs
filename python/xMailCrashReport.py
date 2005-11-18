#!/usr/bin/python
# Alcugs Server Project
# $Id$
#

import smtplib, socket, time, dircache, glob, os, sys

#You can configure this script to suit your needs,
# for example, you may want to delete the coredumps to avoid running out of disk space

def btclean(msg,pgname):
    a=msg.split("\n")
    nmsg=""
    for i in a:
        b=i.split()
        if(len(b)>0):
            c=b[len(b)-1]
            if c.startswith("[") and c.endswith("]"):
                addr=c.strip("[").strip("]")
                os.system("addr2line -e %s %s > btline.tmp" %(pgname,addr))
                f=file("btline.tmp","r")
                nmsg = nmsg + i + " " + f.read()
                f.close()
            else:
                nmsg = nmsg + i + "\n"
        else:
            nmsg = nmsg + i + "\n"
    return nmsg

fromaddr = "\"Alcugs Shard at %s\" <alcugs@localhost>" %(socket.gethostname())
toaddrs  = ["root@localhost",]

# Add the From: and To: headers at the start!
msg = "From: %s\nTo: %s\n" % (fromaddr, ", ".join(toaddrs))
msg = msg + "Date: %s\n" %(time.strftime("%a, %d %b %Y %H:%M:%S +0000", time.gmtime()))
msg = msg + "Subject: Crash Report from Alcugs-shard at %s\n\n" % (socket.gethostname())

msg = msg + """This is an auto-generated crash report.
You can modify xMailCrashReport.py to suit to your needs, and enable dissable it in the uru.conf file

"""

found = 0

pgname="uru_lobby"

for i in sys.argv:
    if i.startswith("name="):
        pgname=i[5:]

cores = glob.glob("*.core") + glob.glob("core")

f=file("gdbcmd.tmp","w")
f.write("bt")
f.close()

for core in cores:
    msg = msg + "%s\n" %(core)
    found=found+1
    os.system("gdb %s -c %s -batch -x gdbcmd.tmp > btline.tmp" %(pgname,core))
    f=file("btline.tmp")
    msg = msg + f.read() + "\n"
    f.close()
    os.remove(core)

msg = msg + "%i coredumps found\n\n" %(found)

found = 0

bts = glob.glob("BackTrace*")


for bt in bts:
    msg = msg + "%s\n" %(bt)
    found=found+1
    os.system("cat %s | c++filt > btline.tmp" %bt)
    f=file("btline.tmp","r")
    msg = msg + btclean(f.read(),pgname)
    f.close()
    os.remove(bt)

msg = msg + "%i Backtraces found\n\n" %(found)

try:
    os.remove("btline.tmp")
    os.remove("gdbcmd.tmp")
except OSError:
    pass

#print msg
#print "Message length is " + repr(len(msg))

#Note: Check the smtplib documentation, if you need to supply auth credentials for your Mail server
server = smtplib.SMTP('localhost')
#server.set_debuglevel(1)
server.sendmail(fromaddr, toaddrs, msg)
server.quit()
