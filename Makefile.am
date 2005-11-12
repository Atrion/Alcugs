SUBDIRS = docs include src

INCLUDES = -I$(top_srcdir)/include/alcugs

#AM_CXXFLAGS = -Wall -rdynamic -g3 -O0
#CXXFLAGS=

lib_LIBRARIES = libalcugsbase.a libalcugsurunet3.a

#LibAlcugsBase library
libalcugsbase_a_SOURCES = \
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
 src/alcbase/urutypes/urubasetypes.cpp
#libalcugsbase_a_LIBADD = alcexcpetion.o

if WIN32
libalcugsbase_a_SOURCES += src/alcbase/alcutil/windoze.cpp
endif

libalcugsbase_a_CPPFLAGS = -DIN_ALC_LIBRARY
#End LibAlcugsBase library

#LibAlcugsUruNet3 library
libalcugsurunet3_a_SOURCES = \
 src/unet/urunet.cpp \
 src/unet/netlog.cpp \
 src/unet/netsession.cpp \
 src/unet/netsessionmgr.cpp \
 src/unet/netmsgq.cpp \
 src/unet/protocol/protocol.cpp \
 src/unet/netcore.cpp \
 src/unet/protocol/umsgbasic.cpp

libalcugsurunet3_a_CPPFLAGS = -DIN_ALC_LIBRARY
#End LibAlcugsUruNet3 library

check_PROGRAMS = test
TESTS = test

#Test APPS
test_LDADD = libalcugsbase.a @ALC_LIBS@

test_SOURCES = src/alcbase/test/test.cpp
#End Test APPS

bin_PROGRAMS = urucrypt m5crypt uruping urumsgtest

#Tools
urucrypt_LDADD = libalcugsbase.a @ALC_LIBS@
m5crypt_LDADD =  libalcugsbase.a @ALC_LIBS@

urucrypt_SOURCES = src/tools/urucrypt.cpp
m5crypt_SOURCES = src/tools/urucrypt.cpp
m5crypt_CPPFLAGS = -DM5CRYPT
#End tools

#Server programs
uruping_LDADD = libalcugsurunet3.a libalcugsbase.a @ALC_LIBS@
urumsgtest_LDADD = libalcugsurunet3.a libalcugsbase.a @ALC_LIBS@

uruping_SOURCES = src/servers/uruping.cpp
urumsgtest_SOURCES = src/servers/urumsgtest.cpp
#End server programs


