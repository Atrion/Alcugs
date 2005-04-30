CC=g++

ifeq ($(DMALLOC),1)
	ARGS=-Wall -g -DDMALLOC_FUNC_CHECK
else
	ARGS=-Wall -g
endif

#the next items are appened to the compilers args
ALC_INCLUDE_PATH=-I/usr/include/python2.2
WIN_ICPREFIX=${MINGWDIR}\\include
WINALC_INCLUDE_PATH=-I${WIN_ICPREFIX}\\python2.2 -I${WIN_ICPREFIX}\\wx\msw
#compiler + args
ifeq ($(WINDOZE),1)
	COMP=${CC} ${ARGS} ${WINALC_INCLUDE_PATH}
else
	COMP=${CC} ${ARGS} ${ALC_INCLUDE_PATH}
endif

#libraryes

#debbuging interface
ifeq ($(DMALLOC),1)
	DBGLIB=dmalloc/libdmallocxx.a
else
	DBGLIB=
endif

# BLIBS=-lm -lcrypto
# THREADLIB=-lpthread
ZLIB=-lz

#cryptography
ifeq ($(WINDOZE),1)
	CRYPTOLIB=
else
	CRYPTOLIB=-lcrypto
endif
#-leay32

#sockets
ifeq ($(WINDOZE),1)
	SOCKETS=-lwsock32
else
	SOCKETS=
endif

#database
ifeq ($(WINDOZE),1)
	DBLIB=-lmysql
else
	DBLIB=-lmysqlclient
endif

#python
ifeq ($(WINDOZE),1)
	PYLIB=-lpython22
else
	PYLIB=-lpython2.2
endif

#wxWidgets
ifeq ($(WINDOZE),1)
	WXFLAGS=-fno-rtti -fno-exceptions -fno-pcc-struct-return -fstrict-aliasing -Wall -D__WXMSW__ -D__GNUWIN32__ -DWINVER=0x400 -D__WIN95__ -DSTRICT
#-fvtable-thunks  -D__WXDEBUG__
	WXLIBS=-lwxmsw -lcomdlg32 -luser32 -lgdi32 -lole32 -lwsock32 -lcomctl32 -lctl3d32 -lgcc -lstdc++ -lshell32 -loleaut32 -ladvapi32 -luuid -lpng -ltiff -ljpeg -lz
else
	WXFLAGS= $(shell wx-config --cxxflags)
	WXLIBS= $(shell wx-config --libs)
endif

#base
BASELIBS=${SOCKETS} ${CRYPTOLIB} ${ZLIB} ${DBGLIB}

#end libs

#sufixes
ifeq ($(WINDOZE),1)
	EXE=.exe
else
	EXE=
endif

#object dependencies
#porting
WIN=md5.o windoze.o

NETCORE=urunet.o protocol.o stdebug.o conv_funs.o license.o version.o\
 gbasicmsg.o useful.o debug.o

CMHS=files.o whatdoyousee.o config_parser.o
SERVEROBJ=settings.o pbasicmsg.o pdefaultmsg.o pnetmsg.o

CAUTHOBJ=pcauthmsg.o gauthmsg.o gscauthmsg.o psauthmsg.o
SAUTHOBJ=gsauthmsg.o pscauthmsg.o cgas_auth.o
AUTH1=auth_db_p.o auth_p.o
AUTH2=auth_db.o auth.o
CVAULTOBJ=psbvaultmsg.o gbvaultmsg.o pcbvaultmsg.o gcsbvaultmsg.o

#vault sub sys
VAULTSS=vault_obj.o vaultsubsys.o vnodes.o gvaultmsg.o htmldumper.o

#python sub system
PYTHONSS=pythonsubsys.o pythonglue.o pythonh.o

AGEPARSER=

#servers
AUTHOBJ=${SERVEROBJ} ${SAUTHOBJ} sql.o settings_db.o
VAULTOBJ=${SERVEROBJ} ${VAULTSS} gsbvaultmsg.o pcsbvaultmsg.o pvaultservermsg.o vserversys.o\
 sql.o settings_db.o vault_db.o ageparser.o guid_gen.o vault_tasks.o vault_advp.o
TRACKINGOBJ=${SERVEROBJ} gtrackingmsg.o pctrackingmsg.o trackingsubsys.o ageparser.o\
 guid_gen.o
LOBBYOBJ=${SERVEROBJ} ${CAUTHOBJ} ${CVAULTOBJ} pclobbymsg.o gctrackingmsg.o globbymsg.o\
 pvaultfordmsg.o pvaultroutermsg.o ${VAULTSS} ptrackingmsg.o lobbysubsys.o
GAMEOBJ=${LOBBYOBJ} gamesubsys.o ${PYTHONSS} ageparser.o pcgamemsg.o pcjgamemsg.o

#base
ifeq ($(WINDOZE),1)
	BASE=${NETCORE} ${CMHS} ${WIN}
else
	BASE=${NETCORE} ${CMHS}
endif
#end objects

#all foo.cpp will be foo.o
%.o: %.cpp %.h
	$(COMP) -c $<

#$@ $<

CLIENT_APP=uruping$(EXE) urucrypt$(EXE)

SERVERS=uru_auth$(EXE) uru_vault$(EXE) uru_tracking$(EXE) uru_lobby$(EXE) uru_game$(EXE)

ALL_PRGS=${CLIENT_APP} ${SERVERS}

all: ${CLIENT_APP} ${SERVERS}

max: ${ALL_PRGS} uru_authp$(EXE) uruproxy$(EXE) urumsgtest$(EXE)

#Server daemons
uru_authp$(EXE): uru.cpp $(BASE) $(AUTHOBJ) $(AUTH1)
	$(COMP) uru.cpp -o uru_authp$(EXE) -DI_AM_THE_AUTH_SERVER $(BASE) $(AUTHOBJ) $(AUTH1)\
	 $(BASELIBS) $(DBLIB)
uru_auth$(EXE): uru.cpp $(BASE) $(AUTHOBJ) $(AUTH2)
	$(COMP) uru.cpp -o uru_auth$(EXE) -DI_AM_THE_AUTH_SERVER $(BASE) $(AUTHOBJ) $(AUTH2)\
	 $(BASELIBS) $(DBLIB)
uru_tracking$(EXE): uru.cpp $(BASE) $(TRACKINGOBJ)
	$(COMP) uru.cpp -o uru_tracking$(EXE) -DI_AM_THE_TRACKING_SERVER $(BASE) $(TRACKINGOBJ) $(BASELIBS)
uru_vault$(EXE): uru.cpp $(BASE) $(VAULTOBJ)
	$(COMP) uru.cpp -o uru_vault$(EXE) -DI_AM_THE_VAULT_SERVER $(BASE) $(VAULTOBJ) $(BASELIBS) $(DBLIB)
uru_lobby$(EXE): uru.cpp $(BASE) $(LOBBYOBJ)
	$(COMP) uru.cpp -o uru_lobby$(EXE) -DI_AM_A_LOBBY_SERVER $(BASE) $(LOBBYOBJ) $(BASELIBS)
uru_game$(EXE): uru.cpp $(BASE) $(GAMEOBJ)
	$(COMP) uru.cpp -o uru_game$(EXE) -DI_AM_A_GAME_SERVER $(BASE) $(GAMEOBJ) $(BASELIBS) $(PYLIB)

#Console app's
uruping$(EXE): uruping.cpp $(BASE)
	$(COMP) uruping.cpp -o uruping$(EXE) $(BASE) $(BASELIBS)
urucrypt$(EXE): urucrypt.cpp files.o whatdoyousee.o license.o
	$(COMP) urucrypt.cpp -o urucrypt$(EXE) files.o whatdoyousee.o license.o
urucmd$(EXE): urucmd.cpp $(BASE)
	$(COMP) urucmd.cpp -o urucmd$(EXE) $(BASE) $(BASELIBS)

#Plasma firewall
plfire$(EXE): plfire.cpp $(BASE) $(VAULTSS)
	$(COMP) plfire.cpp -o plfire$(EXE) $(BASE) $(VAULTSS) $(BASELIBS)

#Alcugs client
gsetup$(EXE): gsetup.cpp gsetup.h
	$(COMP) gsetup.cpp -o gsetup$(EXE) $(WXFLAGS) $(WXLIBS)

#internal app's
uruproxy$(EXE): uruproxy.cpp $(BASE) $(VAULTSS)
	$(COMP) uruproxy.cpp -o uruproxy$(EXE) $(BASE) $(VAULTSS) $(BASELIBS)

#testing app's
urumsgtest$(EXE): urumsgtest.cpp $(BASE)
	$(COMP) urumsgtest.cpp -o urumsgtest$(EXE) $(BASE) $(BASELIBS)

doc:
	doxygen

clean:
	rm -rf *.o *.exe ${ALL_PRGS} docs/source

mrproper:
	rm -rf log *.log *.raw core.*
