# Microsoft Developer Studio Project File - Name="Far" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=Far - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Far.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Far.mak" CFG="Far - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Far - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Far - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Far - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "FAR_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\..\..\SDK" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "FAR_EXPORTS" /Yu"StdAfx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"C:\Program Files\Far\Plugins\7-Zip\7-ZipFar.dll" /opt:NOWIN98
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "Far - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "FAR_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\..\..\SDK" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "FAR_EXPORTS" /Yu"StdAfx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"C:\Program Files\Far\Plugins\7-Zip\7-ZipFar.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "Far - Win32 Release"
# Name "Far - Win32 Debug"
# Begin Group "Spec"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\CLSIDConst.cpp
# End Source File
# Begin Source File

SOURCE=.\Far.def
# End Source File
# Begin Source File

SOURCE=.\Far.rc
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"StdAfx.h"
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\SDK\Common\AlignedBuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Common\IntToString.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Common\IntToString.h
# End Source File
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
# Begin Group "Plugin"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ExtractEngine.cpp
# End Source File
# Begin Source File

SOURCE=.\ExtractEngine.h
# End Source File
# Begin Source File

SOURCE=.\Main.cpp
# End Source File
# Begin Source File

SOURCE=.\Messages.h
# End Source File
# Begin Source File

SOURCE=.\OverwriteDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\OverwriteDialog.h
# End Source File
# Begin Source File

SOURCE=.\Plugin.cpp
# End Source File
# Begin Source File

SOURCE=.\Plugin.h
# End Source File
# Begin Source File

SOURCE=.\PluginCommon.cpp
# End Source File
# Begin Source File

SOURCE=.\PluginDelete.cpp
# End Source File
# Begin Source File

SOURCE=.\PluginRead.cpp
# End Source File
# Begin Source File

SOURCE=.\PluginWrite.cpp
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\UpdateCallback100.cpp
# End Source File
# Begin Source File

SOURCE=.\UpdateCallback100.h
# End Source File
# End Group
# Begin Group "Far"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\SDK\Far\FarPlugin.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Far\FarUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Far\FarUtils.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Far\ProgressBox.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Far\ProgressBox.h
# End Source File
# End Group
# Begin Group "Windows"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\SDK\Windows\COM.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\COM.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\Defs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\Error.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\Error.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\FileDir.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\FileDir.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\FileFind.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\FileFind.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\FileIO.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\FileIO.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\FileName.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\FileName.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\PropVariant.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\PropVariant.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\PropVariantConversions.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\PropVariantConversions.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\Registry.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\Registry.h
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

SOURCE=..\..\..\SDK\Windows\Time.h
# End Source File
# End Group
# Begin Group "Interface"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\SDK\Interface\FileStreams.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Interface\FileStreams.h
# End Source File
# Begin Source File

SOURCE=..\Common\IArchiveHandler2.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Interface\IProgress.h
# End Source File
# End Group
# Begin Group "Archive Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Common\CompressEngineCommon.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\CompressEngineCommon.h
# End Source File
# Begin Source File

SOURCE=..\Common\DefaultName.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\DefaultName.h
# End Source File
# Begin Source File

SOURCE=..\Common\ExtractAutoRename.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\ExtractAutoRename.h
# End Source File
# Begin Source File

SOURCE=..\Common\OpenEngine2.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\OpenEngine2.h
# End Source File
# Begin Source File

SOURCE=..\Common\PropIDUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\PropIDUtils.h
# End Source File
# Begin Source File

SOURCE=..\Common\SortUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\SortUtils.h
# End Source File
# Begin Source File

SOURCE=..\Common\UpdatePairInfo.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\UpdatePairInfo.h
# End Source File
# Begin Source File

SOURCE=..\Common\UpdateProducer.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\UpdateProducer.h
# End Source File
# Begin Source File

SOURCE=..\Common\UpdateUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\UpdateUtils.h
# End Source File
# Begin Source File

SOURCE=..\Common\ZipRegistry.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\ZipRegistry.h
# End Source File
# Begin Source File

SOURCE=..\Common\ZipRegistryMain.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\ZipRegistryMain.h
# End Source File
# Begin Source File

SOURCE=..\Common\ZipSettings.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\ZipSettings.h
# End Source File
# End Group
# Begin Group "Agent"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Agent\AgentProxyHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\Agent\AgentProxyHandler.h
# End Source File
# Begin Source File

SOURCE=..\Agent\ExtractCallback200.cpp
# End Source File
# Begin Source File

SOURCE=..\Agent\ExtractCallback200.h
# End Source File
# Begin Source File

SOURCE=..\Agent\Handler.cpp
# End Source File
# Begin Source File

SOURCE=..\Agent\Handler.h
# End Source File
# Begin Source File

SOURCE=..\Agent\OutHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\Agent\UpdateCallback.cpp
# End Source File
# Begin Source File

SOURCE=..\Agent\UpdateCallback.h
# End Source File
# End Group
# Begin Group "Compression"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\SDK\Compression\CopyCoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Compression\CopyCoder.h
# End Source File
# End Group
# End Target
# End Project
