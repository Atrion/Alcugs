SUBDIRS = include docs

INCLUDES = -I$(top_srcdir)/include/alcbase -I$(top_srcdir)/include/alcnet

ACLOCAL_AMFLAGS = -I m4

#AM_CXXFLAGS = -Wall -rdynamic -g3 -O0
#CXXFLAGS=

lib_LIBRARIES = libalcbase1.a libalcnet1.a

#LibAlcBase library
libalcbase1_a_SOURCES = \
 src/alcbase/alcexception.cpp \
 src/alcbase/alcmain.cpp \
 src/alcbase/alctypes.cpp \
 src/alcbase/alcdebug.cpp \
 src/alcbase/alclicense.cpp \
 src/alcbase/alcversion.cpp \
 src/alcbase/alcutil/alclog.cpp \
 src/alcbase/alcutil/alcos.cpp \
 src/alcbase/alcutil/md5.cpp \
 src/alcbase/alcutil/conv_funs.cpp \
 src/alcbase/alcutil/useful.cpp \
 src/alcbase/alcutil/alccfgtypes.cpp \
 src/alcbase/alcutil/alcparser.cpp \
 src/alcbase/alcutil/rijndael.cpp \
 src/alcbase/urutypes/whatdoyousee.cpp \
 src/alcbase/urutypes/urubasetypes.cpp \
 src/alcbase/alcutil/alcthread.cpp \
 src/alcbase/alcutil/alcpkg.cpp \
 src/alcbase/urutypes/pypkg.cpp

if WIN32
libalcbase1_a_SOURCES += src/alcbase/alcutil/windoze.cpp
endif

libalcbase1_a_CPPFLAGS = -DIN_ALC_LIBRARY
#End LibAlcBase library


#LibAlcUruNet3 library
libalcnet1_a_SOURCES = \
 src/alcnet/alcnet.cpp \
 src/alcnet/netlog.cpp \
 src/alcnet/netsession.cpp \
 src/alcnet/netsessionmgr.cpp \
 src/alcnet/netmsgq.cpp \
 src/alcnet/protocol/protocol.cpp \
 src/alcnet/netcore.cpp \
 src/alcnet/protocol/umsgbasic.cpp \
 src/alcnet/configalias.cpp \
 src/alcnet/unetmain.cpp \
 src/alcnet/unetserverbase.cpp \
 src/alcnet/unetlobbyserverbase.cpp

libalcnet1_a_CPPFLAGS = -DIN_ALC_LIBRARY
#End LibAlcUruNet3 library

#Test APPS
check_PROGRAMS = test test2
TESTS = test test2

test_LDADD = libalcbase1.a 
test_SOURCES = src/alcbase/test/test.cpp

test2_LDADD = libalcbase1.a 
test2_SOURCES = src/alcbase/test/test2.cpp
#End Test APPS

bin_PROGRAMS = alcconfig urucrypt m5crypt alcping alcmsgtest alcugs_lobby

#Tools
urucrypt_LDADD = libalcbase1.a
m5crypt_LDADD =  libalcbase1.a
alcconfig_LDADD = libalcbase1.a

urucrypt_SOURCES = src/tools/urucrypt.cpp
m5crypt_SOURCES = src/tools/urucrypt.cpp
m5crypt_CPPFLAGS = -DM5CRYPT
alcconfig_SOURCES = src/tools/alcconfig.cpp
#End tools

#Server programs
alcping_LDADD = libalcnet1.a libalcbase1.a
alcmsgtest_LDADD = libalcnet1.a libalcbase1.a
alcugs_lobby_LDADD = libalcnet1.a libalcbase1.a

alcping_SOURCES = src/servers/alcping.cpp
alcmsgtest_SOURCES = src/servers/alcmsgtest.cpp

alcugs_lobby_SOURCES = \
 src/servers/alcugs_main.cpp \
 src/servers/lobbyserver.cpp
alcugs_lobby_INCLUDES = \
 src/servers/lobbyserver.h
alcugs_lobby_CPPFLAGS = -DI_AM_THE_LOBBY_SERVER

#End server programs
