# Microsoft Developer Studio Project File - Name="uru_game" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=uru_game - Win32 Debug
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "uru_game.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "uru_game.mak" CFG="uru_game - Win32 Debug"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "uru_game - Win32 Release" (basierend auf  "Win32 (x86) Application")
!MESSAGE "uru_game - Win32 Debug" (basierend auf  "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "uru_game - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "tmp_msvc/uru_game_r"
# PROP Intermediate_Dir "tmp_msvc/uru_game_r"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "C:\uru_server3 build" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "I_AM_A_GAME_SERVER" /D "__WIN32__" /D "__MSVC__" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib Ws2_32.lib zlib.lib /nologo /subsystem:console /machine:I386 /out:"uru_game_r.exe"

!ELSEIF  "$(CFG)" == "uru_game - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "uru_game___Win32_Debug"
# PROP BASE Intermediate_Dir "uru_game___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "tmp_msvc/uru_game_d"
# PROP Intermediate_Dir "tmp_msvc/uru_game_d"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "C:\uru_server3 build" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "I_AM_A_GAME_SERVER" /D "__WIN32__" /D "__MSVC__" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib Ws2_32.lib zlib.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib:"MSVCRT" /out:"uru_game.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "uru_game - Win32 Release"
# Name "uru_game - Win32 Debug"
# Begin Group "Base"

# PROP Default_Filter ""
# Begin Group "NetCore"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\conv_funs.cpp
# End Source File
# Begin Source File

SOURCE=.\conv_funs.h
# End Source File
# Begin Source File

SOURCE=.\debug.cpp
# End Source File
# Begin Source File

SOURCE=.\debug.h
# End Source File
# Begin Source File

SOURCE=.\gbasicmsg.cpp
# End Source File
# Begin Source File

SOURCE=.\gbasicmsg.h
# End Source File
# Begin Source File

SOURCE=.\license.cpp
# End Source File
# Begin Source File

SOURCE=.\license.h
# End Source File
# Begin Source File

SOURCE=.\md5.cpp
# End Source File
# Begin Source File

SOURCE=.\md5.h
# End Source File
# Begin Source File

SOURCE=.\prot.h
# End Source File
# Begin Source File

SOURCE=.\protocol.cpp
# End Source File
# Begin Source File

SOURCE=.\protocol.h
# End Source File
# Begin Source File

SOURCE=.\stdebug.cpp
# End Source File
# Begin Source File

SOURCE=.\stdebug.h
# End Source File
# Begin Source File

SOURCE=.\urunet.cpp
# End Source File
# Begin Source File

SOURCE=.\urunet.h
# End Source File
# Begin Source File

SOURCE=.\useful.cpp
# End Source File
# Begin Source File

SOURCE=.\useful.h
# End Source File
# Begin Source File

SOURCE=.\version.cpp
# End Source File
# Begin Source File

SOURCE=.\version.h
# End Source File
# End Group
# Begin Group "CMHS"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\config_parser.cpp
# End Source File
# Begin Source File

SOURCE=.\config_parser.h
# End Source File
# Begin Source File

SOURCE=.\files.cpp
# End Source File
# Begin Source File

SOURCE=.\files.h
# End Source File
# Begin Source File

SOURCE=.\whatdoyousee.cpp
# End Source File
# Begin Source File

SOURCE=.\whatdoyousee.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\config.h
# End Source File
# Begin Source File

SOURCE=.\windoze.cpp
# End Source File
# Begin Source File

SOURCE=.\windoze.h
# End Source File
# End Group
# Begin Group "GameObj"

# PROP Default_Filter ""
# Begin Group "LobbyObj"

# PROP Default_Filter ""
# Begin Group "ServerObj"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\pbasicmsg.cpp
# End Source File
# Begin Source File

SOURCE=.\pbasicmsg.h
# End Source File
# Begin Source File

SOURCE=.\pdefaultmsg.cpp
# End Source File
# Begin Source File

SOURCE=.\pdefaultmsg.h
# End Source File
# Begin Source File

SOURCE=.\pnetmsg.cpp
# End Source File
# Begin Source File

SOURCE=.\pnetmsg.h
# End Source File
# Begin Source File

SOURCE=.\settings.cpp
# End Source File
# Begin Source File

SOURCE=.\settings.h
# End Source File
# End Group
# Begin Group "cAuthObj"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\gauthmsg.cpp
# End Source File
# Begin Source File

SOURCE=.\gauthmsg.h
# End Source File
# Begin Source File

SOURCE=.\gscauthmsg.cpp
# End Source File
# Begin Source File

SOURCE=.\gscauthmsg.h
# End Source File
# Begin Source File

SOURCE=.\pcauthmsg.cpp
# End Source File
# Begin Source File

SOURCE=.\pcauthmsg.h
# End Source File
# Begin Source File

SOURCE=.\psauthmsg.cpp
# End Source File
# Begin Source File

SOURCE=.\psauthmsg.h
# End Source File
# End Group
# Begin Group "cVaultObj"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\gbvaultmsg.cpp
# End Source File
# Begin Source File

SOURCE=.\gbvaultmsg.h
# End Source File
# Begin Source File

SOURCE=.\gcsbvaultmsg.cpp
# End Source File
# Begin Source File

SOURCE=.\gcsbvaultmsg.h
# End Source File
# Begin Source File

SOURCE=.\pcbvaultmsg.cpp
# End Source File
# Begin Source File

SOURCE=.\pcbvaultmsg.h
# End Source File
# Begin Source File

SOURCE=.\psbvaultmsg.cpp
# End Source File
# Begin Source File

SOURCE=.\psbvaultmsg.h
# End Source File
# End Group
# Begin Group "VaultSS"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\gvaultmsg.cpp
# End Source File
# Begin Source File

SOURCE=.\gvaultmsg.h
# End Source File
# Begin Source File

SOURCE=.\htmldumper.cpp
# End Source File
# Begin Source File

SOURCE=.\htmldumper.h
# End Source File
# Begin Source File

SOURCE=.\vault_obj.cpp
# End Source File
# Begin Source File

SOURCE=.\vault_obj.h
# End Source File
# Begin Source File

SOURCE=.\vaultsubsys.cpp
# End Source File
# Begin Source File

SOURCE=.\vaultsubsys.h
# End Source File
# Begin Source File

SOURCE=.\vnodes.cpp
# End Source File
# Begin Source File

SOURCE=.\vnodes.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\gctrackingmsg.cpp
# End Source File
# Begin Source File

SOURCE=.\gctrackingmsg.h
# End Source File
# Begin Source File

SOURCE=.\globbymsg.cpp
# End Source File
# Begin Source File

SOURCE=.\globbymsg.h
# End Source File
# Begin Source File

SOURCE=.\lobbysubsys.cpp
# End Source File
# Begin Source File

SOURCE=.\lobbysubsys.h
# End Source File
# Begin Source File

SOURCE=.\pclobbymsg.cpp
# End Source File
# Begin Source File

SOURCE=.\pclobbymsg.h
# End Source File
# Begin Source File

SOURCE=.\ptrackingmsg.cpp
# End Source File
# Begin Source File

SOURCE=.\ptrackingmsg.h
# End Source File
# Begin Source File

SOURCE=.\pvaultfordmsg.cpp
# End Source File
# Begin Source File

SOURCE=.\pvaultfordmsg.h
# End Source File
# Begin Source File

SOURCE=.\pvaultroutermsg.cpp
# End Source File
# Begin Source File

SOURCE=.\pvaultroutermsg.h
# End Source File
# End Group
# Begin Group "PythonSS"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\pythonglue.cpp
# End Source File
# Begin Source File

SOURCE=.\pythonglue.h
# End Source File
# Begin Source File

SOURCE=.\pythonh.h
# End Source File
# Begin Source File

SOURCE=.\pythonsubsys.cpp
# End Source File
# Begin Source File

SOURCE=.\pythonsubsys.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\ageparser.cpp
# End Source File
# Begin Source File

SOURCE=.\ageparser.h
# End Source File
# Begin Source File

SOURCE=.\gamesubsys.cpp
# End Source File
# Begin Source File

SOURCE=.\gamesubsys.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\uru.cpp
# End Source File
# Begin Source File

SOURCE=.\uru.h
# End Source File
# End Target
# End Project
