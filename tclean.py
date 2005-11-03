#!/usr/bin/python

import dircache
import os
from os.path import *
import string
import sys


def rmrf(path):
    f=dircache.listdir(path)

    for a in f:
        if isdir(path + "/" + a):
            print "Deleting %s" %(path + "/" +a)
            rmrf(path + "/" + a)
            os.rmdir(path + "/" + a)
        else:
            print "Deleting %s" %(path + "/" + a)
            os.remove(path + "/" + a)

def clean_tree(path):

    f=dircache.listdir(path)

    for a in f:
        if isdir(path + "/" + a):
            if a==".deps":
                print "Deleting %s" %(path + "/" + a)
                rmrf(path + "/" + a)
                os.rmdir(path + "/" + a)
            elif a!=".svn":
                clean_tree(path + "/" + a)
        else:
            if a == "Makefile.in":
                print "Deleting %s" % (path + "/" + a)
                os.remove(path + "/" + a)
            else:
                #if not ((a[-4:] in [".cpp",".sdl",".age"]) or (a[-2:] in [".h"])):
                #    print a[-4:],a[-2:],a
                if a[-5:] in [".core",] or a[-4:] in [".exe",".raw"] or a[-2:] in [".o",".a"] or a[-1:] in ["~",]:
                    print "Deleting %s" % (path + "/" + a)
                    os.remove(path + "/" + a)
                    


clean_tree('.')

