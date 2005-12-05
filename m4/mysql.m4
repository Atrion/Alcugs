#
#    Alcugs Server
#
#    Copyright (C) 2004-2005  The Alcugs Project Server Team
#    See the file AUTHORS for more info about the team
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
#    Please see the file COPYING for the full license.
#    Please see the file DISCLAIMER for more details, before doing anything.
#


# Macro for finding MySQL.

AC_DEFUN([AC_ALC_MYSQL], [

  AC_ARG_WITH([mysql],
    [AS_HELP_STRING([--with-mysql=PREFIX],[use MySQL files in PREFIX])])

  if test x$with_mysql = xno; then
	AC_MSG_ERROR([MySQL is the only database currently supported!])
  elif test x$with_mysql = xyes -o x$with_mysql = x; then
	ac_alc_mysql_prefix=""
  else
	ac_alc_mysql_prefix="$with_mysql"
  fi

  # Now look for the installation
  ac_alc_cached_CPPFLAGS="$CPPFLAGS"
  ac_alc_cached_LDFLAGS="$LDFLAGS"
  MYSQL_CPPFLAGS=""
  MYSQL_LDFLAGS=""
  if test x$ac_alc_mysql_prefix = x; then
	ac_alc_mysql_include="/usr/include /usr/include/mysql /usr/local/include /usr/local/include/mysql"
	ac_alc_mysql_lib="/usr/lib /usr/lib/mysql /usr/local/lib /usr/local/lib/mysql"
	AC_MSG_CHECKING([for mysql installation])
  else
	ac_alc_mysql_include="$ac_alc_mysql_prefix/include $ac_alc_mysql_prefix/include/mysql"
	ac_alc_mysql_lib="$ac_alc_mysql_prefix/lib $ac_alc_mysql_prefix/lib/mysql"
	AC_MSG_CHECKING([for mysql installation in $ac_alc_mysql_prefix])
  fi

  for ac_alc_mysql_dir in $ac_alc_mysql_include; do
	CPPFLAGS="$ac_alc_cached_CPPFLAGS -I$ac_alc_mysql_dir"
	AC_COMPILE_IFELSE([AC_LANG_SOURCE([[#include <mysql.h>]])],[MYSQL_CPPFLAGS="-I$ac_alc_mysql_dir"],[])
	if test ! x$MYSQL_CPPFLAGS = x; then
		break
	fi
  done
  if test x$MYSQL_CPPFLAGS = x; then
	AC_MSG_ERROR([no working MySQL installation found!])
  fi

  for ac_alc_mysql_dir in $ac_alc_mysql_lib; do
	CPPFLAGS="$ac_alc_cached_CPPFLAGS $MYSQL_CPPFLAGS"
	LDFLAGS="$ac_alc_cached_LDFLAGS -L$ac_alc_mysql_dir -lmysqlclient"
	AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <mysql.h>]], [[MYSQL foo; mysql_init(&foo);]])],[
		MYSQL_LDFLAGS="-L$ac_alc_mysql_dir -lmysqlclient"
		break
	  ],[])
  done
  if test "$MYSQL_LDFLAGS" = ""; then
	AC_MSG_ERROR([no working MySQL installation found!])
  fi

  # We found everything, clean up
  AC_MSG_RESULT([yes])
  CPPFLAGS="$ac_alc_cached_CPPFLAGS"
  LDFLAGS="$ac_alc_cached_LDFLAGS"
  AC_SUBST(MYSQL_CPPFLAGS)
  AC_SUBST(MYSQL_LDFLAGS)

]) # AC_ALC_MYSQL
