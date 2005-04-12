# Microsoft Developer Studio Project File - Name="servers" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Generic Project" 0x010a

CFG=servers - Win32 Debug
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "servers.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "servers.mak" CFG="servers - Win32 Debug"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "servers - Win32 Release" (basierend auf  "Win32 (x86) Generic Project")
!MESSAGE "servers - Win32 Debug" (basierend auf  "Win32 (x86) Generic Project")
!MESSAGE 

# Begin Project
# PROP serversowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
MTL=midl.exe

!IF  "$(CFG)" == "servers - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "tmp_msvc"
# PROP Intermediate_Dir "tmp_msvc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "servers - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "tmp_msvc"
# PROP Intermediate_Dir "tmp_msvc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "servers - Win32 Release"
# Name "servers - Win32 Debug"
# Begin Source File

SOURCE=.\config.h
# End Source File
# Begin Source File

SOURCE=.\windoze.cpp
# End Source File
# Begin Source File

SOURCE=.\windoze.h
# End Source File
# End Target
# End Project
