#
#    Alcugs Server
#
#    Copyright (C) 2005-2006  The Alcugs Project Server Team
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


# Macro for specifying where to search for an external utility.
#  I can't believe there isn't something like this already?
# Basically this wraps AC_CHECK_HEADERS and AC_CHECK_LIB with a
#  --with-xxx-dir thrown in. You need to supply an ACTION-IF-FOUND
#  if you want this to do anything other than report success/failure.
#  The ACTION-IF-FOUND and ACTION-IF-NOT-FOUND may refer to
#  $ac_alc_withpath_CPPFLAGS and $ac_alc_withpath_LDFLAGS.
#
# Right now, this looks in the current CPPFLAGS and LDFLAGS before trying
#  anything specified on the command line.

# AC_ALC_WITHPATH(HEADER-FILE, LIBRARY, FUNCTION, [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
#
# AC_CHECK_HEADERS([expat.h], , AC_MSG_FAILURE([expat.h not found!]))
# AC_CHECK_LIB(expat,XML_ParserCreate, , AC_MSG_FAILURE([expat not found!]))
# would then be exactly equivalent to
# AC_ALC_WITHPATH([expat.h],expat,XML_ParserCreate, , AC_MSG_FAILURE([expat not found!]))


AC_DEFUN([AC_ALC_WITHPATH], [

  AC_ARG_WITH([$2],
    [AS_HELP_STRING([--with-$2=PREFIX],[look for $2 files in PREFIX])])

  ac_alc_lib_found=no
  if test x$withval = xyes -o x$withval = x; then
	ac_alc_lib_search=""
  else
	ac_alc_lib_search="$withval"
  fi
  ac_alc_cached_CPPFLAGS="$CPPFLAGS"
  ac_alc_cached_LDFLAGS="$LDFLAGS"
  ac_alc_withpath_CPPFLAGS=""
  ac_alc_withpath_LDFLAGS=""

  AC_COMPILE_IFELSE([AC_LANG_SOURCE([[#include <$1>]])],[ac_alc_lib_found=yes],[])
  if test x$ac_alc_lib_found = xno -a ! x$ac_alc_lib_search = x; then
	ac_alc_withpath_CPPFLAGS="-I${ac_alc_lib_search}/include"
	CPPFLAGS="$CPPFLAGS $ac_alc_withpath_CPPFLAGS"
  fi
  AC_CHECK_HEADERS([$1],
	[ac_alc_lib_found=yes],
	[CPPFLAGS="$ac_alc_cached_CPPFLAGS"
	 m4_default([$5], [AC_MSG_FAILURE([$1 not found!])])])
  if test x$ac_alc_lib_found = xyes; then
    ac_alc_lib_found=no
    AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <$1>]], [[${3}();]])],[ac_alc_lib_found=yes],[])
    if test x$ac_alc_lib_found = xno -a ! x$ac_alc_lib_search = x; then
	ac_alc_withpath_LDFLAGS="-L${ac_alc_lib_search}/lib"
	LDFLAGS="$LDFLAGS $ac_alc_withpath_LDFLAGS"
    fi
    AC_CHECK_LIB([$2], [$3], 
	[CPPFLAGS="$ac_alc_cached_CPPFLAGS"
	 LDFLAGS="$ac_alc_cached_LDFLAGS"
	 m4_default([$4], [])], 
	[CPPFLAGS="$ac_alc_cached_CPPFLAGS"
	 LDFLAGS="$ac_alc_cached_LDFLAGS"
	 m4_default([$5], [AC_MSG_FAILURE([$2 not found!])])])
  fi
]) # AC_ALC_WITHPATH
