# Microsoft Developer Studio Project File - Name="Config" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Config - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Config.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Config.mak" CFG="Config - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Config - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Config - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Config - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /out:"C:\Program Files\7-ZIP\7-zipCfg.exe " /opt:NOWIN98
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "Config - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 htmlhelp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"C:\Program Files\7-ZIP\7-zipCfg.exe " /pdbtype:sept
# SUBTRACT LINK32 /nodefaultlib

!ENDIF 

# Begin Target

# Name "Config - Win32 Release"
# Name "Config - Win32 Debug"
# Begin Group "Spec"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Windows"

# PROP Default_Filter ""
# Begin Group "Control"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\SDK\Windows\Control\Dialog.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\Control\Dialog.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\Control\ListView.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\Control\ListView.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\Control\PropertyPage.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\Control\PropertyPage.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\Control\PropertySheet.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\..\SDK\Windows\COM.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\COM.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\FileName.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\FileName.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\Registry.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\Registry.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\ResourceString.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\ResourceString.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\Shell.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\Shell.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\Synchronization.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\Synchronization.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\System.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\System.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\Window.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\Window.h
# End Source File
# End Group
# Begin Group "Pages"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\FoldersPage.cpp
# End Source File
# Begin Source File

SOURCE=.\FoldersPage.h
# End Source File
# Begin Source File

SOURCE=.\InfoPage.cpp
# End Source File
# Begin Source File

SOURCE=.\InfoPage.h
# End Source File
# Begin Source File

SOURCE=.\SystemPage.cpp
# End Source File
# Begin Source File

SOURCE=.\SystemPage.h
# End Source File
# End Group
# Begin Group "Zip Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Common\HelpUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\HelpUtils.h
# End Source File
# Begin Source File

SOURCE=.\res\Config.ico
# End Source File
# Begin Source File

SOURCE=..\Common\ZipRegistry.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\ZipRegistry.h
# End Source File
# Begin Source File

SOURCE=..\Common\ZipRegistryConfig.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\ZipRegistryConfig.h
# End Source File
# Begin Source File

SOURCE=..\Common\ZipRegistryMain.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\ZipRegistryMain.h
# End Source File
# Begin Source File

SOURCE=..\Common\ZipSettings.h
# End Source File
# End Group
# Begin Group "Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\SDK\Common\String.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Common\StringConvert.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Common\StringConvert.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Common\Types.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Common\Vector.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Common\Vector.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Common\Wildcard.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Common\Wildcard.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\Config.cpp
# End Source File
# Begin Source File

SOURCE=.\Config.ico
# End Source File
# Begin Source File

SOURCE=.\Config.rc
# End Source File
# End Target
# End Project
