CC=g++
ARGS=-Wall -g
# -DDMALLOC_FUNC_CHECK

#the next items are appened to the compilers args
ALC_INCLUDE_PATH=-I/usr/include/python2.2

#compiler + args
COMP=${CC} ${ARGS} ${ALC_INCLUDE_PATH}

#libraryes

#debbuging interface
DBGLIB=
#dmalloc/libdmallocxx.a

# BLIBS=-lm -lcrypto
# THREADLIB=-lpthread
ZLIB=-lz

#cryptography
CRYPTOLIB=-lcrypto
#-leay32

#sockets
WINSOCKETS=-lwsock32

#database
DBLIB=-lmysqlclient
WINDBLIB=-lmysql

#python
PYLIB=-lpython2.2
WINPYLIB=-lpython22

#base
BASELIBS=${CRYPTOLIB} ${ZLIB} ${DBGLIB}
WINBASELIBS=${WINSOCKETS} ${ZLIB}

#end libs

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
BASE=${NETCORE} ${CMHS}
WINBASE=${NETCORE} ${CMHS} ${WIN}

#end objects

#all foo.cpp will be foo.o
%.o: %.cpp %.h
	$(COMP) -c $<

#$@ $<

CLIENT_APP=uruping
WINCLIENT_APP=uruping.exe

SERVERS=uru_auth uru_vault uru_tracking uru_lobby uru_game
WINSERVERS=uru_auth.exe uru_vault.exe uru_tracking.exe uru_lobby.exe uru_game.exe

ALL_PRGS=${CLIENT_APP} ${SERVERS}
WINALL_PRGS=${WINCLIENT_APP} ${WINSERVERS}

all: ${CLIENT_APP} ${SERVERS}

win: ${WINCLIENT_APP} ${WINSERVERS}

max: ${ALL_PRGS} uru_authp uruproxy urumsgtest

uru_authp: uru.cpp $(BASE) $(AUTHOBJ) $(AUTH1)
	$(COMP) uru.cpp -o uru_authp -DI_AM_THE_AUTH_SERVER $(BASE) $(AUTHOBJ) $(AUTH1)\
	 $(BASELIBS) $(DBLIB)
uru_auth: uru.cpp $(BASE) $(AUTHOBJ) $(AUTH2)
	$(COMP) uru.cpp -o uru_auth -DI_AM_THE_AUTH_SERVER $(BASE) $(AUTHOBJ) $(AUTH2)\
	 $(BASELIBS) $(DBLIB)
uru_auth.exe: uru.cpp $(WINBASE) $(AUTHOBJ) $(AUTH2)
	$(COMP) uru.cpp -o uru_auth.exe -DI_AM_THE_AUTH_SERVER $(WINBASE) $(AUTHOBJ) $(AUTH2)\
	$(WINBASELIBS) $(WINDBLIB)

uru_tracking: uru.cpp $(BASE) $(TRACKINGOBJ)
	$(COMP) uru.cpp -o uru_tracking -DI_AM_THE_TRACKING_SERVER $(BASE) $(TRACKINGOBJ) $(BASELIBS)
uru_tracking.exe: uru.cpp $(WINBASE) $(TRACKINGOBJ)
	$(COMP) uru.cpp -o uru_tracking.exe -DI_AM_THE_TRACKING_SERVER $(WINBASE) $(TRACKINGOBJ) $(WINBASELIBS)

uru_vault: uru.cpp $(BASE) $(VAULTOBJ)
	$(COMP) uru.cpp -o uru_vault -DI_AM_THE_VAULT_SERVER $(BASE) $(VAULTOBJ) $(BASELIBS) $(DBLIB)
uru_vault.exe: uru.cpp $(WINBASE) $(VAULTOBJ)
	$(COMP) uru.cpp -o uru_vault.exe -DI_AM_THE_VAULT_SERVER $(WINBASE) $(VAULTOBJ) $(WINBASELIBS) $(WINDBLIB)

uru_lobby: uru.cpp $(BASE) $(LOBBYOBJ)
	$(COMP) uru.cpp -o uru_lobby -DI_AM_A_LOBBY_SERVER $(BASE) $(LOBBYOBJ) $(BASELIBS)
uru_lobby.exe: uru.cpp $(WINBASE) $(LOBBYOBJ)
	$(COMP) uru.cpp -o uru_lobby.exe -DI_AM_A_LOBBY_SERVER $(WINBASE) $(LOBBYOBJ) $(WINBASELIBS)

uru_game: uru.cpp $(BASE) $(GAMEOBJ)
	$(COMP) uru.cpp -o uru_game -DI_AM_A_GAME_SERVER $(BASE) $(GAMEOBJ) $(BASELIBS) $(PYLIB)
uru_game.exe: uru.cpp $(WINBASE) $(GAMEOBJ)
	$(COMP) uru.cpp -o uru_game.exe -DI_AM_A_GAME_SERVER $(WINBASE) $(GAMEOBJ) $(WINBASELIBS) $(WINPYLIB)


uruping: uruping.cpp $(BASE)
	$(COMP) uruping.cpp -o uruping $(BASE) $(BASELIBS)
uruping.exe: uruping.cpp $(WINBASE)
	$(COMP) uruping.cpp -o uruping $(WINBASE) $(WINBASELIBS)

urucrypt: urucrypt.cpp files.o whatdoyousee.o license.o
	$(COMP) urucrypt.cpp -o urucrypt files.o whatdoyousee.o license.o
urucrypt.exe: urucrypt.cpp files.o whatdoyousee.o license.o
	$(COMP) urucrypt.cpp -o urucrypt.exe files.o whatdoyousee.o license.o




#internal app's
uruproxy: uruproxy.cpp $(BASE) $(VAULTSS)
	$(COMP) uruproxy.cpp -o uruproxy $(BASE) $(VAULTSS) $(BASELIBS)

#testing app's
urumsgtest: urumsgtest.cpp $(BASE)
	$(COMP) urumsgtest.cpp -o urumsgtest $(BASE) $(BASELIBS)


doc:
	doxygen

clean:
	rm -rf *.o *.exe ${ALL_PRGS} docs/source

mrproper:
	rm -rf log *.log *.raw core.*
