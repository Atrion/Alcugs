#
# Alcugs Server Project
# $Id$
#

import smtplib

fromaddr = "alcugsserver@localhost"
toaddrs  = ["root@localhost",]

# Add the From: and To: headers at the start!
msg = ("From: %s\r\nTo: %s\r\n\r\n"
       % (fromaddr, ", ".join(toaddrs)))
while 1:
    try:
        line = raw_input()
    except EOFError:
        break
    if not line:
        break
    msg = msg + line

print "Message length is " + repr(len(msg))

server = smtplib.SMTP('matrix.aml.loc')
server.set_debuglevel(1)
server.sendmail(fromaddr, toaddrs, msg)
server.quit()
