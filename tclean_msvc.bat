REM cleans the temporary MSVC files from the tree
REM $Id$

call tclean

del   "*.ilk"             /f /q
del   "*.opt"             /f /q
del   "*.ncb"             /f /q
del   "*.plg"             /f /q
