REM
REM replace c:\MINGW with whatever your installation root may be.
REM GCC_EXEC_PREFIX is optional, and hardly ever needs to be set (read:
REM leave it alone).
REM
SET MINGWDIR=C:\Dev-Cpp
SET WINDOZE=1
PATH=%MINGWDIR%\bin;%PATH%
REM set BISON_SIMPLE=%MINGWDIR%\share\bison.simple
REM set BISON_HAIRY=%MINGWDIR%\share\bison.hairy

REM SET GCC_EXEC_PREFIX=%MINGWDIR%\lib\gcc-lib\
REM set LIBRARY_PATH=%MINGWDIR%\lib;%MINGWDIR%\lib\gcc-lib\i386-mingw32\2.8.1

echo.
echo  Type "make all" to build the servers

cmd
