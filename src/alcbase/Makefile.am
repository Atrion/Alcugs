SUBDIRS = test

INCLUDES = -I$(top_srcdir)/include/alcugs/

#AM_CXXFLAGS = -Wall -rdynamic -g3 -O0
#CXXFLAGS=

lib_LIBRARIES = libalcugsbase.a

libalcugsbase_a_SOURCES = alcexception.cpp alcmain.cpp alctypes.cpp alcdebug.cpp \
 alclicense.cpp alcversion.cpp \
 alcutil/alclog.cpp alcutil/alcos.cpp alcutil/md5.cpp \
 alcutil/conv_funs.cpp alcutil/useful.cpp alcutil/alccfgtypes.cpp\
 alcutil/alcparser.cpp\
 alcutil/rijndael.cpp\
 urutypes/whatdoyousee.cpp urutypes/urubasetypes.cpp
#libalcugsbase_a_LIBADD = alcexcpetion.o

libalcugsbase_a_CPPFLAGS = -DIN_ALC_LIBRARY
