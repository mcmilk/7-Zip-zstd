# Microsoft Developer Studio Project File - Name="GUI" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=GUI - Win32 DebugU
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "GUI.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "GUI.mak" CFG="GUI - Win32 DebugU"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "GUI - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "GUI - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "GUI - Win32 ReleaseU" (based on "Win32 (x86) Application")
!MESSAGE "GUI - Win32 DebugU" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "GUI - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\..\..\SDK" /D "NDEBUG" /D "_MBCS" /D "WIN32" /D "_WINDOWS" /D "LANG" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /out:"C:\Program Files\7-Zip\7zg.exe" /opt:NOWIN98
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "GUI - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\..\SDK" /D "_DEBUG" /D "_MBCS" /D "WIN32" /D "_WINDOWS" /D "LANG" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /out:"C:\Program Files\7-Zip\7zg.exe" /pdbtype:sept

!ELSEIF  "$(CFG)" == "GUI - Win32 ReleaseU"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseU"
# PROP BASE Intermediate_Dir "ReleaseU"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseU"
# PROP Intermediate_Dir "ReleaseU"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\..\..\SDK" /D "NDEBUG" /D "_UNICODE" /D "UNICODE" /D "WIN32" /D "_WINDOWS" /D "LANG" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386 /out:"C:\UTIL\7zg.exe"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /out:"C:\Program Files\7-Zip\7zgn.exe" /opt:NOWIN98
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "GUI - Win32 DebugU"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DebugU"
# PROP BASE Intermediate_Dir "DebugU"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugU"
# PROP Intermediate_Dir "DebugU"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\..\SDK" /D "_DEBUG" /D "_UNICODE" /D "UNICODE" /D "WIN32" /D "_WINDOWS" /D "LANG" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"C:\UTIL\7zg.exe" /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /out:"C:\Program Files\7-Zip\7zgn.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "GUI - Win32 Release"
# Name "GUI - Win32 Debug"
# Name "GUI - Win32 ReleaseU"
# Name "GUI - Win32 DebugU"
# Begin Group "Spec"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\resource.rc
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "SDK"

# PROP Default_Filter ""
# Begin Group "Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\SDK\Common\CommandLineParser.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Common\CommandLineParser.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Common\Defs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Common\Exception.h
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

SOURCE=..\..\..\SDK\Common\ListFileUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Common\ListFileUtils.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Common\NewHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Common\NewHandler.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Common\StdInStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Common\StdInStream.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Common\StdOutStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Common\StdOutStream.h
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

SOURCE=..\..\..\SDK\Common\Types.h
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

SOURCE=..\..\..\SDK\Windows\Control\Edit.h
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

SOURCE=..\..\..\SDK\Windows\Device.h
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

SOURCE=..\..\..\SDK\Windows\Window.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Windows\Window.h
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

SOURCE=..\..\..\SDK\Interface\IInOutStreams.h
# End Source File
# End Group
# Begin Group "Utils"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\SDK\Util\FilePathAutoRename.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Util\FilePathAutoRename.h
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
# End Group
# Begin Group "Archiver Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Common\ArchiveStyleDirItemInfo.h
# End Source File
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

SOURCE=..\Common\OpenEngine200.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\OpenEngine200.h
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

SOURCE=..\Common\ZipSettings.h
# End Source File
# End Group
# Begin Group "Console"

# PROP Default_Filter ""
# End Group
# Begin Group "Explorer"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Explorer\CompressEngine.cpp
# End Source File
# Begin Source File

SOURCE=..\Explorer\CompressEngine.h
# End Source File
# Begin Source File

SOURCE=..\Explorer\ExtractEngine.cpp
# End Source File
# Begin Source File

SOURCE=..\Explorer\ExtractEngine.h
# End Source File
# Begin Source File

SOURCE=..\Explorer\MyMessages.cpp
# End Source File
# Begin Source File

SOURCE=..\Explorer\MyMessages.h
# End Source File
# Begin Source File

SOURCE=..\Explorer\TestEngine.cpp
# End Source File
# Begin Source File

SOURCE=..\Explorer\TestEngine.h
# End Source File
# End Group
# Begin Group "Dialogs"

# PROP Default_Filter ""
# Begin Group "Progress"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\FileManager\Resource\ProgressDialog\ProgressDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\..\FileManager\Resource\ProgressDialog\ProgressDialog.h
# End Source File
# End Group
# Begin Group "Messages"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\FileManager\Resource\MessagesDialog\MessagesDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\..\FileManager\Resource\MessagesDialog\MessagesDialog.h
# End Source File
# End Group
# Begin Group "Overwtite"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\FileManager\Resource\OverwriteDialog\OverwriteDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\..\FileManager\Resource\OverwriteDialog\OverwriteDialog.h
# End Source File
# End Group
# Begin Group "Password"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\FileManager\Resource\PasswordDialog\PasswordDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\..\FileManager\Resource\PasswordDialog\PasswordDialog.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\Explorer\CompressDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\Explorer\CompressDialog.h
# End Source File
# Begin Source File

SOURCE=..\Explorer\ExtractDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\Explorer\ExtractDialog.h
# End Source File
# End Group
# Begin Group "FM Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\FileManager\AppTitle.cpp
# End Source File
# Begin Source File

SOURCE=..\..\FileManager\AppTitle.h
# End Source File
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

SOURCE=..\..\FileManager\OpenCallback.cpp
# End Source File
# Begin Source File

SOURCE=..\..\FileManager\OpenCallback.h
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

SOURCE=..\..\FileManager\StringUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\FileManager\StringUtils.h
# End Source File
# Begin Source File

SOURCE=..\..\FileManager\UpdateCallback100.cpp
# End Source File
# Begin Source File

SOURCE=..\..\FileManager\UpdateCallback100.h
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

SOURCE=..\Agent\ArchiveExtractCallback.cpp
# End Source File
# Begin Source File

SOURCE=..\Agent\ArchiveExtractCallback.h
# End Source File
# Begin Source File

SOURCE=..\Agent\ArchiveUpdateCallback.cpp
# End Source File
# Begin Source File

SOURCE=..\Agent\ArchiveUpdateCallback.h
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
# Begin Group "Archive Interfaces"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Format\Common\ArchiveInterface.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Interface\CompressInterface.h
# End Source File
# Begin Source File

SOURCE=..\..\..\SDK\Interface\CryptoInterface.h
# End Source File
# Begin Source File

SOURCE=..\Common\FolderArchiveInterface.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\GUI.cpp
# End Source File
# End Target
# End Project
