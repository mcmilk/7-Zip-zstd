# Microsoft Developer Studio Project File - Name="Explorer" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=Explorer - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Explorer.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Explorer.mak" CFG="Explorer - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Explorer - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Explorer - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Explorer - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "EXPLORER_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "EXPLORER_EXPORTS" /Yu"StdAfx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib htmlhelp.lib /nologo /dll /machine:I386 /out:"C:\Program Files\7-ZIP\7-Zip.dll" /opt:NOWIN98
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "Explorer - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "EXPLORER_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "EXPLORER_EXPORTS" /Yu"StdAfx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib htmlhelp.lib /nologo /dll /debug /machine:I386 /out:"C:\Program Files\7-ZIP\7-Zip.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "Explorer - Win32 Release"
# Name "Explorer - Win32 Debug"
# Begin Group "Spec"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\DllExports.cpp
# End Source File
# Begin Source File

SOURCE=.\Explorer.def
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\resource.rc
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"StdAfx.h"
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

SOURCE=..\..\..\SDK\Windows\Control\ComboBox.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\Control\ComboBox.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\Control\Dialog.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\Control\Dialog.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\Control\ImageList.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\Control\ImageList.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\Control\ListView.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\Control\ListView.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\Control\ProgressBar.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\Control\Static.h
# End Source File
# End Group
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

SOURCE=..\..\..\SDK\Windows\ItemIDListUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\ItemIDListUtils.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\Menu.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\NationalTime.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\NationalTime.h
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
# Begin Group "Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\SDK\Common\IntToString.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Common\IntToString.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Common\NewHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Common\NewHandler.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Common\Random.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Common\Random.h
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
# Begin Group "Archiver Common"

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

SOURCE=..\Common\HelpUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\HelpUtils.h
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

SOURCE=..\Common\UpdatePairBasic.h
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
# Begin Group "Interface"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\SDK\Interface\FileStreams.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Interface\FileStreams.h
# End Source File
# Begin Source File

SOURCE=..\Format\Common\IArchiveHandler.h
# End Source File
# Begin Source File

SOURCE=..\Common\IArchiveHandler2.h
# End Source File
# End Group
# Begin Group "Engine"

# PROP Default_Filter ""
# Begin Group "ZipView"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ZipEnum.cpp
# End Source File
# Begin Source File

SOURCE=.\ZipEnum.h
# End Source File
# Begin Source File

SOURCE=.\ZipFolder.cpp
# End Source File
# Begin Source File

SOURCE=.\ZipFolder.h
# End Source File
# Begin Source File

SOURCE=.\ZipViewDelete.cpp
# End Source File
# Begin Source File

SOURCE=.\ZipViewExtract.cpp
# End Source File
# Begin Source File

SOURCE=.\ZipViewObject.cpp
# End Source File
# Begin Source File

SOURCE=.\ZipViewObject.h
# End Source File
# Begin Source File

SOURCE=.\ZipViewOpen.cpp
# End Source File
# Begin Source File

SOURCE=.\ZipViewUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\ZipViewUtils.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\CompressEngine.cpp
# End Source File
# Begin Source File

SOURCE=.\CompressEngine.h
# End Source File
# Begin Source File

SOURCE=.\ContextMenu.cpp
# End Source File
# Begin Source File

SOURCE=.\ContextMenu.h
# End Source File
# Begin Source File

SOURCE=.\DeleteEngine.cpp
# End Source File
# Begin Source File

SOURCE=.\DeleteEngine.h
# End Source File
# Begin Source File

SOURCE=.\ExtractCallback.cpp
# End Source File
# Begin Source File

SOURCE=.\ExtractCallback.h
# End Source File
# Begin Source File

SOURCE=.\ExtractEngine.cpp
# End Source File
# Begin Source File

SOURCE=.\ExtractEngine.h
# End Source File
# Begin Source File

SOURCE=.\ExtractIcon.cpp
# End Source File
# Begin Source File

SOURCE=.\ExtractIcon.h
# End Source File
# Begin Source File

SOURCE=.\FormatUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\FormatUtils.h
# End Source File
# Begin Source File

SOURCE=.\HandlersManager.cpp
# End Source File
# Begin Source File

SOURCE=.\HandlersManager.h
# End Source File
# Begin Source File

SOURCE=.\MyIDList.cpp
# End Source File
# Begin Source File

SOURCE=.\MyIDList.h
# End Source File
# Begin Source File

SOURCE=.\MyMessages.cpp
# End Source File
# Begin Source File

SOURCE=.\MyMessages.h
# End Source File
# Begin Source File

SOURCE=.\ProcessMessages.cpp
# End Source File
# Begin Source File

SOURCE=.\ProcessMessages.h
# End Source File
# Begin Source File

SOURCE=.\ProxyHandler.cpp
# End Source File
# Begin Source File

SOURCE=.\ProxyHandler.h
# End Source File
# Begin Source File

SOURCE=.\UpdateCallback100.cpp
# End Source File
# Begin Source File

SOURCE=.\UpdateCallback100.h
# End Source File
# End Group
# Begin Group "Dialogs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ColumnsDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\ColumnsDialog.h
# End Source File
# Begin Source File

SOURCE=.\CompressDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\CompressDialog.h
# End Source File
# Begin Source File

SOURCE=.\ExtractDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\ExtractDialog.h
# End Source File
# Begin Source File

SOURCE=.\MessagesDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\MessagesDialog.h
# End Source File
# Begin Source File

SOURCE=.\OverwriteDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\OverwriteDialog.h
# End Source File
# Begin Source File

SOURCE=.\PasswordDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\PasswordDialog.h
# End Source File
# Begin Source File

SOURCE=.\ProgressDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\ProgressDialog.h
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
