# Microsoft Developer Studio Project File - Name="FM" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=FM - Win32 DebugU
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "FM.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "FM.mak" CFG="FM - Win32 DebugU"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "FM - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "FM - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "FM - Win32 ReleaseU" (based on "Win32 (x86) Application")
!MESSAGE "FM - Win32 DebugU" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "FM - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /Gz /MD /W3 /GX /O1 /I "..\..\SDK" /D "NDEBUG" /D "_MBCS" /D "WIN32" /D "_WINDOWS" /D "LANG" /Yu"StdAfx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib Mpr.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /out:"C:\Program Files\7-ZIP\7zFM.exe" /opt:NOWIN98
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "FM - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /Gz /W3 /Gm /GX /ZI /Od /I "..\..\SDK" /D "_DEBUG" /D "_MBCS" /D "WIN32" /D "_WINDOWS" /D "LANG" /Yu"StdAfx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib Mpr.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /out:"C:\Program Files\7-ZIP\7zFM.exe" /pdbtype:sept

!ELSEIF  "$(CFG)" == "FM - Win32 ReleaseU"

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
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"StdAfx.h" /FD /c
# ADD CPP /nologo /Gz /MD /W3 /GX /O1 /I "..\..\SDK" /D "NDEBUG" /D "_UNICODE" /D "UNICODE" /D "WIN32" /D "_WINDOWS" /D "LANG" /Yu"StdAfx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib Mpr.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /out:"C:\Program Files\7-ZIP\7zFMn.exe" /opt:NOWIN98
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "FM - Win32 DebugU"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"StdAfx.h" /FD /GZ /c
# ADD CPP /nologo /Gz /MDd /W3 /Gm /GX /ZI /Od /I "..\..\SDK" /D "_DEBUG" /D "_UNICODE" /D "UNICODE" /D "WIN32" /D "_WINDOWS" /D "LANG" /Yu"StdAfx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib Mpr.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /out:"C:\Program Files\7-ZIP\7zFMn.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "FM - Win32 Release"
# Name "FM - Win32 Debug"
# Name "FM - Win32 ReleaseU"
# Name "FM - Win32 DebugU"
# Begin Group "Spec"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ClassDefs.cpp
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"StdAfx.h"
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Archive Interfaces"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Archiver\Format\Common\ArchiveInterface.h
# End Source File
# End Group
# Begin Group "Folders"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\FolderInterface.h
# End Source File
# Begin Source File

SOURCE=.\FSDrives.cpp
# End Source File
# Begin Source File

SOURCE=.\FSDrives.h
# End Source File
# Begin Source File

SOURCE=.\FSFolder.cpp
# End Source File
# Begin Source File

SOURCE=.\FSFolder.h
# End Source File
# Begin Source File

SOURCE=.\FSFolderCopy.cpp
# End Source File
# Begin Source File

SOURCE=.\NetFolder.cpp
# End Source File
# Begin Source File

SOURCE=.\NetFolder.h
# End Source File
# Begin Source File

SOURCE=.\RootFolder.cpp
# End Source File
# Begin Source File

SOURCE=.\RootFolder.h
# End Source File
# End Group
# Begin Group "Registry"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\RegistryAssociations.cpp
# End Source File
# Begin Source File

SOURCE=.\RegistryAssociations.h
# End Source File
# Begin Source File

SOURCE=.\RegistryPlugins.cpp
# End Source File
# Begin Source File

SOURCE=.\RegistryPlugins.h
# End Source File
# Begin Source File

SOURCE=.\RegistryUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\RegistryUtils.h
# End Source File
# Begin Source File

SOURCE=.\ViewSettings.cpp
# End Source File
# Begin Source File

SOURCE=.\ViewSettings.h
# End Source File
# End Group
# Begin Group "Panel"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\App.cpp
# End Source File
# Begin Source File

SOURCE=.\App.h
# End Source File
# Begin Source File

SOURCE=.\AppState.h
# End Source File
# Begin Source File

SOURCE=.\FileFolderPluginOpen.cpp
# End Source File
# Begin Source File

SOURCE=.\FileFolderPluginOpen.h
# End Source File
# Begin Source File

SOURCE=.\Panel.cpp
# End Source File
# Begin Source File

SOURCE=.\Panel.h
# End Source File
# Begin Source File

SOURCE=.\PanelFolderChange.cpp
# End Source File
# Begin Source File

SOURCE=.\PanelItemOpen.cpp
# End Source File
# Begin Source File

SOURCE=.\PanelItems.cpp
# End Source File
# Begin Source File

SOURCE=.\PanelKey.cpp
# End Source File
# Begin Source File

SOURCE=.\PanelListNotify.cpp
# End Source File
# Begin Source File

SOURCE=.\PanelMenu.cpp
# End Source File
# Begin Source File

SOURCE=.\PanelOperations.cpp
# End Source File
# Begin Source File

SOURCE=.\PanelSelect.cpp
# End Source File
# Begin Source File

SOURCE=.\PanelSort.cpp
# End Source File
# End Group
# Begin Group "Dialog"

# PROP Default_Filter ""
# Begin Group "Options"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Resource\EditPage\EditPage.cpp
# End Source File
# Begin Source File

SOURCE=.\Resource\EditPage\EditPage.h
# End Source File
# Begin Source File

SOURCE=.\Resource\LangPage\LangPage.cpp
# End Source File
# Begin Source File

SOURCE=.\Resource\LangPage\LangPage.h
# End Source File
# Begin Source File

SOURCE=.\Resource\PluginsPage\PluginsPage.cpp
# End Source File
# Begin Source File

SOURCE=.\Resource\PluginsPage\PluginsPage.h
# End Source File
# Begin Source File

SOURCE=.\Resource\SettingsPage\SettingsPage.cpp
# End Source File
# Begin Source File

SOURCE=.\Resource\SettingsPage\SettingsPage.h
# End Source File
# Begin Source File

SOURCE=.\Resource\SystemPage\SystemPage.cpp
# End Source File
# Begin Source File

SOURCE=.\Resource\SystemPage\SystemPage.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\Resource\AboutDialog\AboutDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\Resource\AboutDialog\AboutDialog.h
# End Source File
# Begin Source File

SOURCE=.\Resource\ComboDialog\ComboDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\Resource\ComboDialog\ComboDialog.h
# End Source File
# Begin Source File

SOURCE=.\Resource\CopyDialog\CopyDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\Resource\CopyDialog\CopyDialog.h
# End Source File
# Begin Source File

SOURCE=.\Resource\ListViewDialog\ListViewDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\Resource\ListViewDialog\ListViewDialog.h
# End Source File
# Begin Source File

SOURCE=Resource\MessagesDialog\MessagesDialog.cpp
# End Source File
# Begin Source File

SOURCE=Resource\MessagesDialog\MessagesDialog.h
# End Source File
# Begin Source File

SOURCE=Resource\OverwriteDialog\OverwriteDialog.cpp
# End Source File
# Begin Source File

SOURCE=Resource\OverwriteDialog\OverwriteDialog.h
# End Source File
# Begin Source File

SOURCE=Resource\PasswordDialog\PasswordDialog.cpp
# End Source File
# Begin Source File

SOURCE=Resource\PasswordDialog\PasswordDialog.h
# End Source File
# Begin Source File

SOURCE=Resource\ProgressDialog\ProgressDialog.cpp
# End Source File
# Begin Source File

SOURCE=Resource\ProgressDialog\ProgressDialog.h
# End Source File
# End Group
# Begin Group "FM Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ExtractCallback.cpp
# End Source File
# Begin Source File

SOURCE=.\ExtractCallback.h
# End Source File
# Begin Source File

SOURCE=.\FormatUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\FormatUtils.h
# End Source File
# Begin Source File

SOURCE=.\HelpUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\HelpUtils.h
# End Source File
# Begin Source File

SOURCE=.\LangUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\LangUtils.h
# End Source File
# Begin Source File

SOURCE=.\ProgramLocation.cpp
# End Source File
# Begin Source File

SOURCE=.\ProgramLocation.h
# End Source File
# Begin Source File

SOURCE=.\UpdateCallback100.cpp
# End Source File
# Begin Source File

SOURCE=.\UpdateCallback100.h
# End Source File
# End Group
# Begin Group "SDK"

# PROP Default_Filter ""
# Begin Group "Windows"

# PROP Default_Filter ""
# Begin Group "Control"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\SDK\Windows\Control\ComboBox.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\Control\ComboBox.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\Control\CommandBar.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\Control\Dialog.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\Control\Dialog.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\Control\Edit.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\Control\ListView.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\Control\ListView.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\Control\PropertyPage.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\Control\PropertyPage.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\Control\ReBar.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\Control\Static.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\Control\StatusBar.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\Control\ToolBar.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\Control\Window2.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\Control\Window2.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\SDK\Windows\COM.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\COM.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\COMTry.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\Defs.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\Error.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\Error.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\FileDir.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\FileDir.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\FileFind.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\FileFind.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\FileIO.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\FileIO.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\FileName.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\FileName.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\FileSystem.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\FileSystem.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\Handle.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\Menu.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\Net.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\Net.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\PropVariant.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\PropVariant.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\PropVariantConversions.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\PropVariantConversions.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\Registry.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\Registry.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\ResourceString.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\ResourceString.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\Shell.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\Shell.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\Synchronization.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\Synchronization.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\System.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\System.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\Thread.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\Window.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Windows\Window.h
# End Source File
# End Group
# Begin Group "Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\SDK\Common\Buffer.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Common\Defs.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Common\IntToString.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Common\IntToString.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Common\Lang.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Common\Lang.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Common\NewHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Common\NewHandler.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Common\StdInStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Common\StdInStream.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Common\StdOutStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Common\StdOutStream.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Common\String.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Common\String.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Common\StringConvert.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Common\StringConvert.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Common\TextConfig.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Common\TextConfig.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Common\UTFConvert.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Common\UTFConvert.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Common\Vector.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Common\Vector.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Common\Wildcard.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Common\Wildcard.h
# End Source File
# End Group
# Begin Group "Interface"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\SDK\Interface\EnumStatProp.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Interface\EnumStatProp.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Interface\FileStreams.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Interface\FileStreams.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Interface\ICoder.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Interface\IInOutStreams.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Interface\IProgress.h
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Interface\PropID.h
# End Source File
# End Group
# Begin Group "Compression"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\SDK\Compression\CopyCoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Compression\CopyCoder.h
# End Source File
# End Group
# Begin Group "Util"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\SDK\Util\FilePathAutoRename.cpp
# End Source File
# Begin Source File

SOURCE=..\..\SDK\Util\FilePathAutoRename.h
# End Source File
# End Group
# End Group
# Begin Source File

SOURCE=.\FilePlugins.cpp
# End Source File
# Begin Source File

SOURCE=.\FilePlugins.h
# End Source File
# Begin Source File

SOURCE=.\FM.cpp
# End Source File
# Begin Source File

SOURCE=.\FM.ico
# End Source File
# Begin Source File

SOURCE=.\MyLoadMenu.cpp
# End Source File
# Begin Source File

SOURCE=.\MyLoadMenu.h
# End Source File
# Begin Source File

SOURCE=.\OpenCallback.cpp
# End Source File
# Begin Source File

SOURCE=.\OpenCallback.h
# End Source File
# Begin Source File

SOURCE=.\OptionsDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\Parent.bmp
# End Source File
# Begin Source File

SOURCE=.\PluginInterface.h
# End Source File
# Begin Source File

SOURCE=.\PropertyName.cpp
# End Source File
# Begin Source File

SOURCE=.\PropertyName.h
# End Source File
# Begin Source File

SOURCE=.\resource.rc
# ADD BASE RSC /l 0x419
# ADD RSC /l 0x409
# End Source File
# Begin Source File

SOURCE=.\StringUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\StringUtils.h
# End Source File
# Begin Source File

SOURCE=.\SysIconUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\SysIconUtils.h
# End Source File
# Begin Source File

SOURCE=.\TextPairs.cpp
# End Source File
# Begin Source File

SOURCE=.\TextPairs.h
# End Source File
# End Target
# End Project
