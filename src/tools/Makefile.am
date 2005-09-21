INCLUDES = -I$(top_srcdir)/include/alcugs/

LDADD = $(top_srcdir)/src/alcbase/libalcugsbase.a \
        @ALC_LIBS@

bin_PROGRAMS = urucrypt m5crypt
urucrypt_SOURCES = urucrypt.cpp
m5crypt_SOURCES = urucrypt.cpp
m5crypt_CPPFLAGS = -DM5CRYPT
