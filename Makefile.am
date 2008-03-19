SUBDIRS = include docs

INCLUDES = -I$(top_srcdir)/include/alcugs

ACLOCAL_AMFLAGS = -I m4

#AM_CXXFLAGS = -Wall -rdynamic -g3 -O0
#CXXFLAGS=

lib_LIBRARIES = libalcugsbase1.a libalcugsnet1.a

#LibAlcugsBase library
libalcugsbase1_a_SOURCES = \
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
libalcugsbase1_a_SOURCES += src/alcbase/alcutil/windoze.cpp
endif

libalcugsbase1_a_CPPFLAGS = -DIN_ALC_LIBRARY
#End LibAlcugsBase library


#LibAlcugsUruNet3 library
libalcugsnet1_a_SOURCES = \
 src/unet/alcnet.cpp \
 src/unet/netlog.cpp \
 src/unet/netsession.cpp \
 src/unet/netsessionmgr.cpp \
 src/unet/netmsgq.cpp \
 src/unet/protocol/protocol.cpp \
 src/unet/netcore.cpp \
 src/unet/protocol/umsgbasic.cpp \
 src/unet/configalias.cpp \
 src/unet/unetmain.cpp \
 src/unet/unetserverbase.cpp \
 src/unet/unetlobbyserverbase.cpp

libalcugsnet1_a_CPPFLAGS = -DIN_ALC_LIBRARY
#End LibAlcugsUruNet3 library

#Test APPS
check_PROGRAMS = test test2
TESTS = test test2

test_LDADD = libalcugsbase1.a 
test_SOURCES = src/alcbase/test/test.cpp

test2_LDADD = libalcugsbase1.a 
test2_SOURCES = src/alcbase/test/test2.cpp
#End Test APPS

bin_PROGRAMS = alcconfig urucrypt m5crypt alcping alcmsgtest alcugs_lobby

#Tools
urucrypt_LDADD = libalcugsbase1.a
m5crypt_LDADD =  libalcugsbase1.a
alcconfig_LDADD = libalcugsbase1.a

urucrypt_SOURCES = src/tools/urucrypt.cpp
m5crypt_SOURCES = src/tools/urucrypt.cpp
m5crypt_CPPFLAGS = -DM5CRYPT
alcconfig_SOURCES = src/tools/alcconfig.cpp
#End tools

#Server programs
alcping_LDADD = libalcugsnet1.a libalcugsbase1.a
alcmsgtest_LDADD = libalcugsnet1.a libalcugsbase1.a
alcugs_lobby_LDADD = libalcugsnet1.a libalcugsbase1.a

alcping_SOURCES = src/servers/alcping.cpp
alcmsgtest_SOURCES = src/servers/alcmsgtest.cpp

alcugs_lobby_SOURCES = \
 src/servers/alcugs_main.cpp \
 src/servers/lobbyserver.cpp
alcugs_lobby_INCLUDES = \
 src/servers/lobbyserver.h
alcugs_lobby_CPPFLAGS = -DI_AM_THE_LOBBY_SERVER

#End server programs
