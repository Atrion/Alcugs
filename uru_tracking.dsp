# Microsoft Developer Studio Project File - Name="uru_tracking" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=uru_tracking - Win32 Debug
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "uru_tracking.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "uru_tracking.mak" CFG="uru_tracking - Win32 Debug"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "uru_tracking - Win32 Release" (basierend auf  "Win32 (x86) Application")
!MESSAGE "uru_tracking - Win32 Debug" (basierend auf  "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "uru_tracking - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "tmp_msvc/uru_tracking_r"
# PROP Intermediate_Dir "tmp_msvc/uru_tracking_r"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "C:\uru_server3 build" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "I_AM_THE_TRACKING_SERVER" /D "__WIN32__" /D "__MSVC__" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib Ws2_32.lib zlib.lib /nologo /subsystem:console /machine:I386 /out:"uru_tracking_r.exe"

!ELSEIF  "$(CFG)" == "uru_tracking - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "uru_tracking___Win32_Debug"
# PROP BASE Intermediate_Dir "uru_tracking___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "tmp_msvc/uru_tracking_d"
# PROP Intermediate_Dir "tmp_msvc/uru_tracking_d"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "C:\uru_server3 build" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "I_AM_THE_TRACKING_SERVER" /D "__WIN32__" /D "__MSVC__" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib Ws2_32.lib zlib.lib /nologo /subsystem:console /debug /machine:I386 /out:"uru_tracking.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "uru_tracking - Win32 Release"
# Name "uru_tracking - Win32 Debug"
# Begin Group "Base"

# PROP Default_Filter ""
# Begin Group "NetCore"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\conv_funs.cpp

!IF  "$(CFG)" == "uru_tracking - Win32 Release"

!ELSEIF  "$(CFG)" == "uru_tracking - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\conv_funs.h

!IF  "$(CFG)" == "uru_tracking - Win32 Release"

!ELSEIF  "$(CFG)" == "uru_tracking - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\debug.cpp

!IF  "$(CFG)" == "uru_tracking - Win32 Release"

!ELSEIF  "$(CFG)" == "uru_tracking - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\debug.h

!IF  "$(CFG)" == "uru_tracking - Win32 Release"

!ELSEIF  "$(CFG)" == "uru_tracking - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gbasicmsg.cpp

!IF  "$(CFG)" == "uru_tracking - Win32 Release"

!ELSEIF  "$(CFG)" == "uru_tracking - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\gbasicmsg.h

!IF  "$(CFG)" == "uru_tracking - Win32 Release"

!ELSEIF  "$(CFG)" == "uru_tracking - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\license.cpp

!IF  "$(CFG)" == "uru_tracking - Win32 Release"

!ELSEIF  "$(CFG)" == "uru_tracking - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\license.h

!IF  "$(CFG)" == "uru_tracking - Win32 Release"

!ELSEIF  "$(CFG)" == "uru_tracking - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\md5.cpp

!IF  "$(CFG)" == "uru_tracking - Win32 Release"

!ELSEIF  "$(CFG)" == "uru_tracking - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\md5.h

!IF  "$(CFG)" == "uru_tracking - Win32 Release"

!ELSEIF  "$(CFG)" == "uru_tracking - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\prot.h

!IF  "$(CFG)" == "uru_tracking - Win32 Release"

!ELSEIF  "$(CFG)" == "uru_tracking - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\protocol.cpp

!IF  "$(CFG)" == "uru_tracking - Win32 Release"

!ELSEIF  "$(CFG)" == "uru_tracking - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\protocol.h

!IF  "$(CFG)" == "uru_tracking - Win32 Release"

!ELSEIF  "$(CFG)" == "uru_tracking - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\stdebug.cpp

!IF  "$(CFG)" == "uru_tracking - Win32 Release"

!ELSEIF  "$(CFG)" == "uru_tracking - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\stdebug.h

!IF  "$(CFG)" == "uru_tracking - Win32 Release"

!ELSEIF  "$(CFG)" == "uru_tracking - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\urunet.cpp

!IF  "$(CFG)" == "uru_tracking - Win32 Release"

!ELSEIF  "$(CFG)" == "uru_tracking - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\urunet.h

!IF  "$(CFG)" == "uru_tracking - Win32 Release"

!ELSEIF  "$(CFG)" == "uru_tracking - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\useful.cpp

!IF  "$(CFG)" == "uru_tracking - Win32 Release"

!ELSEIF  "$(CFG)" == "uru_tracking - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\useful.h

!IF  "$(CFG)" == "uru_tracking - Win32 Release"

!ELSEIF  "$(CFG)" == "uru_tracking - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\version.cpp

!IF  "$(CFG)" == "uru_tracking - Win32 Release"

!ELSEIF  "$(CFG)" == "uru_tracking - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\version.h

!IF  "$(CFG)" == "uru_tracking - Win32 Release"

!ELSEIF  "$(CFG)" == "uru_tracking - Win32 Debug"

!ENDIF 

# End Source File
# End Group
# Begin Group "CMHS"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\config_parser.cpp

!IF  "$(CFG)" == "uru_tracking - Win32 Release"

!ELSEIF  "$(CFG)" == "uru_tracking - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\config_parser.h

!IF  "$(CFG)" == "uru_tracking - Win32 Release"

!ELSEIF  "$(CFG)" == "uru_tracking - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\files.cpp

!IF  "$(CFG)" == "uru_tracking - Win32 Release"

!ELSEIF  "$(CFG)" == "uru_tracking - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\files.h

!IF  "$(CFG)" == "uru_tracking - Win32 Release"

!ELSEIF  "$(CFG)" == "uru_tracking - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\whatdoyousee.cpp

!IF  "$(CFG)" == "uru_tracking - Win32 Release"

!ELSEIF  "$(CFG)" == "uru_tracking - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\whatdoyousee.h

!IF  "$(CFG)" == "uru_tracking - Win32 Release"

!ELSEIF  "$(CFG)" == "uru_tracking - Win32 Debug"

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\config.h

!IF  "$(CFG)" == "uru_tracking - Win32 Release"

!ELSEIF  "$(CFG)" == "uru_tracking - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\windoze.cpp

!IF  "$(CFG)" == "uru_tracking - Win32 Release"

!ELSEIF  "$(CFG)" == "uru_tracking - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\windoze.h

!IF  "$(CFG)" == "uru_tracking - Win32 Release"

!ELSEIF  "$(CFG)" == "uru_tracking - Win32 Debug"

!ENDIF 

# End Source File
# End Group
# Begin Group "TrackingObj"

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
# Begin Source File

SOURCE=.\ageparser.cpp
# End Source File
# Begin Source File

SOURCE=.\ageparser.h
# End Source File
# Begin Source File

SOURCE=.\gtrackingmsg.cpp
# End Source File
# Begin Source File

SOURCE=.\gtrackingmsg.h
# End Source File
# Begin Source File

SOURCE=.\guid_gen.cpp
# End Source File
# Begin Source File

SOURCE=.\guid_gen.h
# End Source File
# Begin Source File

SOURCE=.\pctrackingmsg.cpp
# End Source File
# Begin Source File

SOURCE=.\pctrackingmsg.h
# End Source File
# Begin Source File

SOURCE=.\trackingsubsys.cpp
# End Source File
# Begin Source File

SOURCE=.\trackingsubsys.h
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
