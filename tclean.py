#!/usr/bin/python

import dircache
import os
from os.path import *
import string
import sys


def clean_tree(path):

    f=dircache.listdir(path)

    for a in f:
        if isdir(path + "/" + a) and a!=".svn":
            clean_tree(path + "/" + a)
        else:
            if a == "Makefile.in":
                print "Deleting %s" % (path + "/" + a)
                os.remove(path + "/" + a)
            else:
                #if not ((a[-4:] in [".cpp",".sdl",".age"]) or (a[-2:] in [".h"])):
                #    print a[-4:],a[-2:],a
                if a[-4:] in [".exe",] or a[-2:] in [".o",] or a[-1:] in ["~",]:
                    print "Deleting %s" % (path + "/" + a)
                    os.remove(path + "/" + a)
                    


clean_tree('.')

