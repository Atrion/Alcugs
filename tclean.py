#!/usr/bin/python

import dircache
import os
from os.path import *
import string
import sys


def clean_tree(path):

    f=dircache.listdir(path)

    for a in f:
        if isdir(path + "/" + a):
            clean_tree(path + "/" + a)
        else:
            if a == "Makefile.in":
                print "Deleting %s" % (path + "/" + a)
                os.remove(path + "/" + a)


clean_tree('.')

