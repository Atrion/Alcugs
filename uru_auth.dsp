# Microsoft Developer Studio Project File - Name="uru_auth" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=uru_auth - Win32 Debug
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "uru_auth.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "uru_auth.mak" CFG="uru_auth - Win32 Debug"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "uru_auth - Win32 Release" (basierend auf  "Win32 (x86) Application")
!MESSAGE "uru_auth - Win32 Debug" (basierend auf  "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "uru_auth - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "tmp_msvc/uru_auth_r"
# PROP Intermediate_Dir "tmp_msvc/uru_auth_r"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "C:\uru_server3 build" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "I_AM_THE_AUTH_SERVER" /D "__WIN32__" /D "__MSVC__" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib Ws2_32.lib zlib.lib libmysql.lib NetCore.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "uru_auth - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "tmp_msvc/uru_auth_d"
# PROP Intermediate_Dir "tmp_msvc/uru_auth_d"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "C:\uru_server3 build" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "I_AM_THE_AUTH_SERVER" /D "__WIN32__" /D "__MSVC__" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib Ws2_32.lib zlib.lib libmysql.lib /nologo /subsystem:console /debug /machine:I386 /out:"uru_auth.exe" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "uru_auth - Win32 Release"
# Name "uru_auth - Win32 Debug"
# Begin Group "Base"

# PROP Default_Filter ""
# Begin Group "NetCore"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\conv_funs.cpp

!IF  "$(CFG)" == "uru_auth - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "uru_auth - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\conv_funs.h

!IF  "$(CFG)" == "uru_auth - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "uru_auth - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\debug.cpp

!IF  "$(CFG)" == "uru_auth - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "uru_auth - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\debug.h

!IF  "$(CFG)" == "uru_auth - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "uru_auth - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gbasicmsg.cpp

!IF  "$(CFG)" == "uru_auth - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "uru_auth - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gbasicmsg.h

!IF  "$(CFG)" == "uru_auth - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "uru_auth - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\license.cpp

!IF  "$(CFG)" == "uru_auth - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "uru_auth - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\license.h

!IF  "$(CFG)" == "uru_auth - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "uru_auth - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\md5.cpp
# End Source File
# Begin Source File

SOURCE=.\md5.h
# End Source File
# Begin Source File

SOURCE=.\prot.h

!IF  "$(CFG)" == "uru_auth - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "uru_auth - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\protocol.cpp

!IF  "$(CFG)" == "uru_auth - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "uru_auth - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\protocol.h

!IF  "$(CFG)" == "uru_auth - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "uru_auth - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\stdebug.cpp

!IF  "$(CFG)" == "uru_auth - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "uru_auth - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\stdebug.h

!IF  "$(CFG)" == "uru_auth - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "uru_auth - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\urunet.cpp

!IF  "$(CFG)" == "uru_auth - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "uru_auth - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\urunet.h

!IF  "$(CFG)" == "uru_auth - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "uru_auth - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\useful.cpp

!IF  "$(CFG)" == "uru_auth - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "uru_auth - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\useful.h

!IF  "$(CFG)" == "uru_auth - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "uru_auth - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\version.cpp

!IF  "$(CFG)" == "uru_auth - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "uru_auth - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\version.h

!IF  "$(CFG)" == "uru_auth - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "uru_auth - Win32 Debug"

!ENDIF 

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
# Begin Group "AuthObj"

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
# Begin Group "sAuthObj"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\cgas_auth.cpp
# End Source File
# Begin Source File

SOURCE=.\cgas_auth.h
# End Source File
# Begin Source File

SOURCE=.\gsauthmsg.cpp
# End Source File
# Begin Source File

SOURCE=.\gsauthmsg.h
# End Source File
# Begin Source File

SOURCE=.\pscauthmsg.cpp
# End Source File
# Begin Source File

SOURCE=.\pscauthmsg.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\settings_db.cpp
# End Source File
# Begin Source File

SOURCE=.\settings_db.h
# End Source File
# Begin Source File

SOURCE=.\sql.cpp
# End Source File
# Begin Source File

SOURCE=.\sql.h
# End Source File
# End Group
# Begin Group "Auth"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\auth.cpp
# End Source File
# Begin Source File

SOURCE=.\auth.h
# End Source File
# Begin Source File

SOURCE=.\auth_db.cpp
# End Source File
# Begin Source File

SOURCE=.\auth_db.h
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
