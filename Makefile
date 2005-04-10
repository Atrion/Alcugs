CC=g++
ARGS=-Wall -g
BLIBS=-lm -lcrypto
THREADLIB=-lpthread
ZLIB=-lz

#mysql location
LDBLIBS=-lmysqlclient
WINDBLIBS=/usr/local/lib/mysql/libmysqlclient.a


DBLIBS=${LDBLIBS}

FILE_OBJS=files.o whatdoyousee.o

URUSDL_OBJS=files.o whatdoyousee.o conv_funs.o adv_gamemsgparser.o stdebug.o \
		sdlparser.o config_parser.o license.o tmp_config.o

PRP_OBJS= license.o conv_funs.o adv_gamemsgparser.o useful.o

COMMON_OBJS= stdebug.o urunet.o protocol.o conv_funs.o config_parser.o files.o \
		useful.o msg_parser.o vault_obj.o sdlparser.o whatdoyousee.o \
		adv_gamemsgparser.o license.o ageparser.o tmp_config.o
URUAUTH_OBJS=	vnodes.o cgas_auth.o urumsg.o pservermsg.o pclientmsg.o pdefaultmsg.o \
		pscauthmsg.o psauthmsg.o htmldumper.o auth_db.o sql.o
URUPING_OBJS=	urumsg.o vnodes.o
URUCMD_OBJS=	urumsg.o vnodes.o pclientmsg.o
URUTRACK_OBJS=	pservermsg.o pclientmsg.o pdefaultmsg.o pscauthmsg.o pstrackingmsg.o \
		vnodes.o urumsg.o htmldumper.o guid_gen.o
URUMETA_OBJS= pservermsg.o pclientmsg.o pdefaultmsg.o pscauthmsg.o psmetamsg.o \
		urumsg.o meta_db.o sql.o htmldumper.o vnodes.o
URULOB_OBJS=	pservermsg.o pclientmsg.o pdefaultmsg.o pscauthmsg.o pstrackingmsg.o \
		vnodes.o urumsg.o \
		htmldumper.o guid_gen.o
URUVAULT_OBJS=	pservermsg.o pclientmsg.o pdefaultmsg.o pscauthmsg.o pstrackingmsg.o \
		psvaultmsg.o vault_db.o vault_tasks.o vault_advp.o guid_gen.o \
		vnodes.o urumsg.o htmldumper.o sql.o
URUGAME_OBJS=	pservermsg.o pclientmsg.o pdefaultmsg.o pscauthmsg.o vnodes.o \
		psgamemsg.o urumsg.o htmldumper.o
URUCONVERT_OBJS = urumsg.o vnodes.o pclientmsg.o mysql.o log.o
LIBS=-lm -lcrypto -lpthread ${ZLIB}

SERVERS= uru_auth uru_tracking uru_vault uru_lobby uru_game
OSERVERS= uru_meta
WINSERVERS= uru_auth.exe uru_tracking.exe uru_vault.exe \
		uru_lobby.exe uru_game.exe
TOOLS= uruping urucmd urusdl urucrypt uruprp uruprppatch
TDEVEL= uruage

all: ${SERVERS} ${TOOLS}

servers: ${SERVERS}
tools: ${TOOLS}

max: ${SERVERS} ${TOOLS} ${OSERVERS} ${TDEVEL}

license.o: license.cpp license.h
	${CC} ${ARGS} -c license.cpp

whatdoyousee.o: whatdoyousee.cpp whatdoyousee.h
	${CC} ${ARGS} -c whatdoyousee.cpp

files.o: files.cpp files.h whatdoyousee.h
	${CC} ${ARGS} -c files.cpp

config_parser.o: config_parser.cpp config_parser.h data_types.h conv_funs.h
	${CC} ${ARGS} -c config_parser.cpp

conv_funs.o: conv_funs.cpp conv_funs.h data_types.h
	${CC} ${ARGS} -c conv_funs.cpp

injector.o: injector.cpp injector.h
	${CC} ${ARGS} -c injector.cpp

stdebug.o: stdebug.cpp stdebug.h
	${CC} ${ARGS} -c stdebug.cpp

cgas_auth.o: cgas_auth.cpp cgas_auth.h
	${CC} ${ARGS} -c cgas_auth.cpp

tmp_config.o: tmp_config.cpp tmp_config.h
	${CC} ${ARGS} -c tmp_config.cpp

urumsg.o: urumsg.cpp urumsg.h
	${CC} ${ARGS} -c urumsg.cpp

sql.o: sql.cpp sql.h
	${CC} ${ARGS} -c sql.cpp

auth_db.o: auth_db.cpp auth_db.h
	${CC} ${ARGS} -c auth_db.cpp

vault_db.o: vault_db.cpp vault_db.h
	${CC} ${ARGS} -c vault_db.cpp

meta_db.o: meta_db.cpp meta_db.h
	${CC} ${ARGS} -c meta_db.cpp

vault_obj.o: vault_obj.cpp vault_obj.h
	${CC} ${ARGS} -c vault_obj.cpp

vault_tasks.o: vault_tasks.cpp vault_tasks.h
	${CC} ${ARGS} -c vault_tasks.cpp

vault_advp.o: vault_advp.cpp vault_advp.h
	${CC} ${ARGS} -c vault_advp.cpp

vnodes.o: vnodes.cpp vnodes.h
	${CC} ${ARGS} -c vnodes.cpp

htmldumper.o: htmldumper.cpp htmldumper.h
	${CC} ${ARGS} -c htmldumper.cpp

guid_gen.o: guid_gen.cpp guid_gen.h
	${CC} ${ARGS} -c guid_gen.cpp

adv_gamemsgparser.o: adv_gamemsgparser.cpp adv_gamemsgparser.h
	${CC} ${ARGS} -c adv_gamemsgparser.cpp

useful.o: useful.cpp useful.h
	${CC} ${ARGS} -c useful.cpp

protocol.o: protocol.cpp protocol.h data_types.h conv_funs.h config_parser.h stdebug.h \
	conv_funs.h prot.h
	${CC} ${ARGS} -c protocol.cpp

urunet.o: urunet.cpp urunet.h config_parser.h stdebug.h conv_funs.h protocol.h prot.h \
	files.h files.cpp
	${CC} ${ARGS} -c urunet.cpp

msg_parser.o: msg_parser.cpp msg_parser.h
	${CC} ${ARGS} -c msg_parser.cpp

pclientmsg.o: pclientmsg.cpp pclientmsg.h
	${CC} ${ARGS} -c pclientmsg.cpp

pdefaultmsg.o: pdefaultmsg.cpp pdefaultmsg.h
	${CC} ${ARGS} -c pdefaultmsg.cpp

psauthmsg.o: psauthmsg.cpp psauthmsg.h
	${CC} ${ARGS} -c psauthmsg.cpp

pscauthmsg.o: pscauthmsg.cpp pscauthmsg.h
	${CC} ${ARGS} -c pscauthmsg.cpp

pservermsg.o: pservermsg.cpp pservermsg.h
	${CC} ${ARGS} -c pservermsg.cpp

psgamemsg.o: psgamemsg.cpp psgamemsg.h
	${CC} ${ARGS} -c psgamemsg.cpp

pstrackingmsg.o: pstrackingmsg.cpp pstrackingmsg.h
	${CC} ${ARGS} -c pstrackingmsg.cpp

psvaultmsg.o: psvaultmsg.cpp psvaultmsg.h
	${CC} ${ARGS} -c psvaultmsg.cpp

psmetamsg.o: psmetamsg.cpp psmetamsg.h
	${CC} ${ARGS} -c psmetamsg.cpp

sdlparser.o: sdlparser.cpp sdlparser.h
	${CC} ${ARGS} -c sdlparser.cpp

ageparser.o: ageparser.cpp ageparser.h
	${CC} ${ARGS} -c ageparser.cpp

mysql.o: mysql.cpp mysql.h
	${CC} ${ARGS} -c mysql.cpp

log.o: log.cpp log.h
	${CC} ${ARGS} -c log.cpp

uruping: uruping.cpp ${COMMON_OBJS} ${URUPING_OBJS}
	${CC} ${ARGS} uruping.cpp ${COMMON_OBJS} ${URUPING_OBJS} ${LIBS} -o uruping

urucmd: urucmd.cpp ${COMMON_OBJS} ${URUCMD_OBJS}
	${CC} ${ARGS} urucmd.cpp ${COMMON_OBJS} ${URUCMD_OBJS} ${LIBS} -o urucmd

uruconvert: uruconvert.cpp ${COMMON_OBJS} ${URUCONVERT_OBJS}
	${CC} ${ARGS} uruconvert.cpp ${COMMON_OBJS} ${URUCONVERT_OBJS} ${LIBS} ${LDBLIBS} -o uruconvert

uru_lobby: uru.cpp ${COMMON_OBJS} ${URULOB_OBJS}
	${CC} ${ARGS} uru.cpp -DI_AM_A_LOBBY_SERVER ${COMMON_OBJS} ${URULOB_OBJS} ${LIBS} -o uru_lobby

uru_game: uru.cpp ${COMMON_OBJS} ${URUGAME_OBJS}
	${CC} ${ARGS} uru.cpp -DI_AM_A_GAME_SERVER ${COMMON_OBJS} ${URUGAME_OBJS} ${LIBS} -o uru_game

uru_auth: uru.cpp ${COMMON_OBJS} ${URUAUTH_OBJS}
	${CC} ${ARGS} uru.cpp -DI_AM_THE_AUTH_SERVER ${COMMON_OBJS} ${URUAUTH_OBJS} ${LIBS} ${DBLIBS} -o uru_auth

uru_vault: uru.cpp ${COMMON_OBJS} ${URUVAULT_OBJS}
	${CC} ${ARGS} uru.cpp -DI_AM_THE_VAULT_SERVER ${COMMON_OBJS} ${URUVAULT_OBJS} ${LIBS} ${DBLIBS} -o uru_vault

uru_tracking: uru.cpp ${COMMON_OBJS} ${URUTRACK_OBJS} pstrackingmsg.cpp
	${CC} ${ARGS} uru.cpp -DI_AM_THE_TRACKING_SERVER ${COMMON_OBJS} ${URUTRACK_OBJS} ${LIBS} -o uru_tracking

# uru_lobby.exe: uru.cpp ${CLIENT_REQ} ${SERVER_REQ}
# 	${CC} ${DBG2} uru.cpp ${CLIENT_LIBS} -o uru_lobby.exe
#
# uru_auth2: uru.cpp ${CLIENT_REQ} ${SERVER_REQ} auth_server.cpp ${DB_REQ} ${AUTH_REQ}
# 	${CC} ${DBG2} uru.cpp -D_INTERNAL_STUFF_AUTH -DI_AM_THE_AUTH_SERVER ${CLIENT_LIBS} -lmysqlclient -o uru_auth2

uru_meta: uru.cpp ${COMMON_OBJS} ${URUMETA_OBJS} psmetamsg.cpp
	${CC} ${ARGS} uru.cpp -DI_AM_THE_META_SERVER ${COMMON_OBJS} ${URUMETA_OBJS} ${LIBS} -lmysqlclient -o uru_meta

urusdl: urusdl.cpp ${URUSDL_OBJS}
	${CC} ${ARGS} urusdl.cpp ${URUSDL_OBJS} ${BLIBS} ${ZLIB} -o urusdl

uruage: ageparser.cpp ${FILE_OBJS}
	${CC} ${ARGS} ageparser.cpp -D_AGE_PARSER_TEST_ ${FILE_OBJS} ${BLIBS} ${ZLIB} -o uruage

urucrypt: urucrypt.cpp ${FILE_OBJS}
	${CC} ${ARGS} urucrypt.cpp license.o ${FILE_OBJS} ${BLIBS} -o urucrypt

uruprp: uruprp.cpp ${FILE_OBJS} ${PRP_OBJS}
	${CC} ${ARGS} uruprp.cpp ${PRP_OBJS} ${FILE_OBJS} ${BLIBS} -o uruprp

uruprppatch: uruprp.cpp ${FILE_OBJS} ${PRP_OBJS}
	${CC} ${ARGS} uruprp.cpp -DAUTOMATIC_PATCH ${PRP_OBJS} ${FILE_OBJS} ${BLIBS} -o uruprppatch

install:
	./install.sh normal ./normal

devel:
	./install.sh devel

doc:
	doxygen

#remove all this garbage
clean:
	rm -rf *.o ${SERVERS} ${TOOLS} ${TDEVEL} ${OSERVERS} docs/source *.exe

purgue:
	./purgue.sh

mrproper:
	./tclean.sh

