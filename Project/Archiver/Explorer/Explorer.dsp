# Microsoft Developer Studio Project File - Name="Explorer" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=Explorer - Win32 DebugU
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Explorer.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Explorer.mak" CFG="Explorer - Win32 DebugU"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Explorer - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Explorer - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Explorer - Win32 ReleaseU" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Explorer - Win32 DebugU" (based on "Win32 (x86) Dynamic-Link Library")
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
# ADD CPP /nologo /MD /W3 /GX /O1 /I "..\..\..\SDK" /D "NDEBUG" /D "_MBCS" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "EXPLORER_EXPORTS" /D "LANG" /D "NEW_FOLDER_INTERFACE" /Yu"StdAfx.h" /FD /c
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
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\..\..\SDK" /D "_DEBUG" /D "_MBCS" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "EXPLORER_EXPORTS" /D "LANG" /D "NEW_FOLDER_INTERFACE" /Yu"StdAfx.h" /FD /GZ /c
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

!ELSEIF  "$(CFG)" == "Explorer - Win32 ReleaseU"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseU"
# PROP BASE Intermediate_Dir "ReleaseU"
# PROP BASE Ignore_Export_Lib 1
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseU"
# PROP Intermediate_Dir "ReleaseU"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "EXPLORER_EXPORTS" /D "LANG" /D "_MBCS" /Yu"StdAfx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /I "..\..\..\SDK" /D "NDEBUG" /D "_UNICODE" /D "UNICODE" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "EXPLORER_EXPORTS" /D "LANG" /D "NEW_FOLDER_INTERFACE" /Yu"StdAfx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib htmlhelp.lib /nologo /dll /machine:I386 /out:"C:\Program Files\7-ZIP\7-Zip.dll" /opt:NOWIN98
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib htmlhelp.lib /nologo /dll /machine:I386 /out:"C:\Program Files\7-ZIP\7-Zipn.dll" /opt:NOWIN98
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "Explorer - Win32 DebugU"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DebugU"
# PROP BASE Intermediate_Dir "DebugU"
# PROP BASE Ignore_Export_Lib 1
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugU"
# PROP Intermediate_Dir "DebugU"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "EXPLORER_EXPORTS" /D "LANG" /D "_MBCS" /Yu"StdAfx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\..\..\SDK" /D "_DEBUG" /D "_UNICODE" /D "UNICODE" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "EXPLORER_EXPORTS" /D "LANG" /D "NEW_FOLDER_INTERFACE" /Yu"StdAfx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib htmlhelp.lib /nologo /dll /debug /machine:I386 /out:"C:\Program Files\7-ZIP\7-Zip.dll" /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib htmlhelp.lib /nologo /dll /debug /machine:I386 /out:"C:\Program Files\7-ZIP\7-Zipn.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "Explorer - Win32 Release"
# Name "Explorer - Win32 Debug"
# Name "Explorer - Win32 ReleaseU"
# Name "Explorer - Win32 DebugU"
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
# Begin Group "Engine"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ContextMenu.cpp
# End Source File
# Begin Source File

SOURCE=.\ContextMenu.h
# End Source File
# Begin Source File

SOURCE=.\MyMessages.cpp
# End Source File
# Begin Source File

SOURCE=.\MyMessages.h
# End Source File
# End Group
# Begin Group "Dialogs"

# PROP Default_Filter ""
# Begin Group "FileManager Dialogs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\FileManager\Resource\MessagesDialog\MessagesDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\..\FileManager\Resource\MessagesDialog\MessagesDialog.h
# End Source File
# Begin Source File

SOURCE=..\..\FileManager\Resource\OverwriteDialog\OverwriteDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\..\FileManager\Resource\OverwriteDialog\OverwriteDialog.h
# End Source File
# Begin Source File

SOURCE=..\..\FileManager\Resource\PasswordDialog\PasswordDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\..\FileManager\Resource\PasswordDialog\PasswordDialog.h
# End Source File
# Begin Source File

SOURCE=..\..\FileManager\Resource\ProgressDialog\ProgressDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\..\FileManager\Resource\ProgressDialog\ProgressDialog.h
# End Source File
# End Group
# Begin Group "Options"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Resource\FoldersPage\FoldersPage.cpp
# End Source File
# Begin Source File

SOURCE=..\Resource\FoldersPage\FoldersPage.h
# End Source File
# Begin Source File

SOURCE=..\Resource\SystemPage\SystemPage.cpp
# End Source File
# Begin Source File

SOURCE=..\Resource\SystemPage\SystemPage.h
# End Source File
# End Group
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

SOURCE=..\Agent\ArchiveExtractCallback.cpp
# End Source File
# Begin Source File

SOURCE=..\Agent\ArchiveExtractCallback.h
# End Source File
# Begin Source File

SOURCE=..\Agent\ArchiveFolder.cpp
# End Source File
# Begin Source File

SOURCE=..\Agent\ArchiveFolderOpen.cpp
# End Source File
# Begin Source File

SOURCE=..\Agent\ArchiveUpdateCallback.cpp
# End Source File
# Begin Source File

SOURCE=..\Agent\ArchiveUpdateCallback.h
# End Source File
# Begin Source File

SOURCE=..\Agent\FolderOut.cpp
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
# End Group
# Begin Group "Spec Interfaces"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Common\ArchiveStyleDirItemInfo.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Interface\CompressInterface.h
# End Source File
# Begin Source File

SOURCE=..\Common\FolderArchiveInterface.h
# End Source File
# Begin Source File

SOURCE=..\Format\Common\FormatCryptoInterface.h
# End Source File
# End Group
# Begin Group "FileManager"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\FileManager\ExtractCallback.cpp
# End Source File
# Begin Source File

SOURCE=..\..\FileManager\ExtractCallback.h
# End Source File
# Begin Source File

SOURCE=..\..\FileManager\FolderInterface.h
# End Source File
# Begin Source File

SOURCE=..\..\FileManager\FormatUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\FileManager\FormatUtils.h
# End Source File
# Begin Source File

SOURCE=..\..\FileManager\HelpUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\FileManager\HelpUtils.h
# End Source File
# Begin Source File

SOURCE=..\..\FileManager\LangUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\FileManager\LangUtils.h
# End Source File
# Begin Source File

SOURCE=..\..\FileManager\ProgramLocation.cpp
# End Source File
# Begin Source File

SOURCE=..\..\FileManager\ProgramLocation.h
# End Source File
# Begin Source File

SOURCE=..\..\FileManager\RegistryUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\FileManager\RegistryUtils.h
# End Source File
# Begin Source File

SOURCE=..\..\FileManager\UpdateCallback100.cpp
# End Source File
# Begin Source File

SOURCE=..\..\FileManager\UpdateCallback100.h
# End Source File
# End Group
# Begin Group "SDK"

# PROP Default_Filter ""
# Begin Group "Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\SDK\Common\Defs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Common\IntToString.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Common\IntToString.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Common\Lang.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Common\Lang.h
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

SOURCE=..\..\..\SDK\Common\StdInStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Common\StdInStream.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Common\String.cpp
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

SOURCE=..\..\..\SDK\Common\TextConfig.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Common\TextConfig.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Common\UTFConvert.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Common\UTFConvert.h
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

SOURCE=..\..\..\SDK\Windows\Control\PropertyPage.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\Control\PropertyPage.h
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

SOURCE=..\..\..\SDK\Windows\FileMapping.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\FileMapping.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\FileName.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\FileName.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\Handle.h
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

SOURCE=..\..\..\SDK\Windows\Thread.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\Window.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\Window.h
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
# Begin Group "Interface"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\SDK\Interface\EnumStatProp.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Interface\EnumStatProp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Interface\FileStreams.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Interface\FileStreams.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Interface\ICoder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Interface\IInOutStreams.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Interface\IProgress.h
# End Source File
# Begin Source File

SOURCE=..\..\FileManager\PluginInterface.h
# End Source File
# End Group
# Begin Group "Util"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\SDK\Util\FilePathAutoRename.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Util\FilePathAutoRename.h
# End Source File
# End Group
# End Group
# Begin Source File

SOURCE=.\OptionsDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\OptionsDialog.h
# End Source File
# End Target
# End Project
