#
#    Alcugs Server
#
#    Copyright (C) 2006  The Alcugs Project Server Team
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

# Macro to test whether unaligned accesses fail. If they do,
#  NEED_STRICT_ALIGNMENT is defined.

AC_DEFUN([AC_ALC_ALIGN], [
  AC_CACHE_CHECK(
   [for alignment requirement],
   ac_alc_need_align,
   [
      case "${host}" in
      *-*-cygwin* | *-*-mingw32*)
	# I am doing this because I have NO idea if the test code I am writing
	# will work in Windows. If you are running Windows on hardware that
	# requires strict alignment, ouch. (Does such a beast exist?)
	ac_alc_need_align=no
	;;
      *)
	AC_RUN_IFELSE(
	   [AC_LANG_SOURCE([[
		#include <signal.h>
		void bushandler(int s) { exit(1); }
		int main() {
			unsigned char data[5] = { 0x0, 0x1, 0x2, 0x3, 0x4 };
			unsigned short foo;
			signal(SIGBUS, bushandler);
			foo = *((unsigned short *)(data+1));
			exit(0);
		}
	   ]])],
	   [ac_alc_need_align=no],
	   [ac_alc_need_align=yes],
	   [ac_alc_need_align=yes])
	;;
      esac
   ])dnl end AC_CACHE_CHECK

  if test x$ac_alc_need_align = xyes; then
	AC_DEFINE(NEED_STRICT_ALIGNMENT,1,[Define to 1 if unaligned pointer dereferences fail.])
  fi
]) # AC_ALC_ALIGN
