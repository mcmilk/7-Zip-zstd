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
# ADD CPP /nologo /Gz /MD /W3 /GX /O1 /I "..\..\\" /D "NDEBUG" /D "_MBCS" /D "WIN32" /D "_WINDOWS" /D "LANG" /Yu"StdAfx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib Mpr.lib htmlhelp.lib Urlmon.lib /nologo /subsystem:windows /machine:I386 /out:"C:\Program Files\7-ZIP\7zFM.exe" /opt:NOWIN98
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
# ADD CPP /nologo /Gz /W3 /Gm /GX /ZI /Od /I "..\..\\" /D "_DEBUG" /D "_MBCS" /D "WIN32" /D "_WINDOWS" /D "LANG" /Yu"StdAfx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib Mpr.lib htmlhelp.lib Urlmon.lib /nologo /subsystem:windows /debug /machine:I386 /out:"C:\Program Files\7-ZIP\7zFM.exe" /pdbtype:sept

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
# ADD CPP /nologo /Gz /MD /W3 /GX /O1 /I "..\..\\" /D "NDEBUG" /D "_UNICODE" /D "UNICODE" /D "WIN32" /D "_WINDOWS" /D "LANG" /Yu"StdAfx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib Mpr.lib htmlhelp.lib Urlmon.lib /nologo /subsystem:windows /machine:I386 /out:"C:\Program Files\7-ZIP\7zFMn.exe" /opt:NOWIN98
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
# ADD CPP /nologo /Gz /MDd /W3 /Gm /GX /ZI /Od /I "..\..\\" /D "_DEBUG" /D "_UNICODE" /D "UNICODE" /D "WIN32" /D "_WINDOWS" /D "LANG" /Yu"StdAfx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib Mpr.lib htmlhelp.lib Urlmon.lib /nologo /subsystem:windows /debug /machine:I386 /out:"C:\Program Files\7-ZIP\7zFMn.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "FM - Win32 Release"
# Name "FM - Win32 Debug"
# Name "FM - Win32 ReleaseU"
# Name "FM - Win32 DebugU"
# Begin Group "Spec"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Resource\AboutDialog\7zipLogo.ico
# End Source File
# Begin Source File

SOURCE=.\add.bmp
# End Source File
# Begin Source File

SOURCE=.\ClassDefs.cpp
# End Source File
# Begin Source File

SOURCE=.\Copy.bmp
# End Source File
# Begin Source File

SOURCE=.\Delete.bmp
# End Source File
# Begin Source File

SOURCE=.\Extract.bmp
# End Source File
# Begin Source File

SOURCE=.\FM.ico
# End Source File
# Begin Source File

SOURCE=.\Move.bmp
# End Source File
# Begin Source File

SOURCE=.\Parent.bmp
# End Source File
# Begin Source File

SOURCE=.\Properties.bmp
# End Source File
# Begin Source File

SOURCE=.\resource.rc
# ADD BASE RSC /l 0x419
# ADD RSC /l 0x409
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"StdAfx.h"
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\Test.bmp
# End Source File
# End Group
# Begin Group "Archive Interfaces"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Archive\IArchive.h
# End Source File
# Begin Source File

SOURCE=..\UI\Agent\IFolderArchive.h
# End Source File
# End Group
# Begin Group "Folders"

# PROP Default_Filter ""
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

SOURCE=.\IFolder.h
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
# Begin Source File

SOURCE=.\PanelSplitFile.cpp
# End Source File
# End Group
# Begin Group "Dialog"

# PROP Default_Filter ""
# Begin Group "Options"

# PROP Default_Filter ""
# Begin Group "Settings"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Resource\SettingsPage\resource.h
# End Source File
# Begin Source File

SOURCE=.\Resource\SettingsPage\resource.rc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\Resource\SettingsPage\SettingsPage.cpp
# End Source File
# Begin Source File

SOURCE=.\Resource\SettingsPage\SettingsPage.h
# End Source File
# End Group
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

SOURCE=.\Resource\SystemPage\SystemPage.cpp
# End Source File
# Begin Source File

SOURCE=.\Resource\SystemPage\SystemPage.h
# End Source File
# End Group
# Begin Group "Password"

# PROP Default_Filter ""
# Begin Source File

SOURCE=Resource\PasswordDialog\PasswordDialog.cpp
# End Source File
# Begin Source File

SOURCE=Resource\PasswordDialog\PasswordDialog.h
# End Source File
# Begin Source File

SOURCE=.\Resource\PasswordDialog\resource.h
# End Source File
# Begin Source File

SOURCE=.\Resource\PasswordDialog\resource.rc
# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "Progress"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Resource\ProgressDialog2\ProgressDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\Resource\ProgressDialog2\ProgressDialog.h
# End Source File
# Begin Source File

SOURCE=.\Resource\ProgressDialog2\resource.h
# End Source File
# Begin Source File

SOURCE=.\Resource\ProgressDialog2\resource.rc
# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "About"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Resource\AboutDialog\AboutDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\Resource\AboutDialog\AboutDialog.h
# End Source File
# Begin Source File

SOURCE=.\Resource\AboutDialog\resource.h
# End Source File
# Begin Source File

SOURCE=.\Resource\AboutDialog\resource.rc
# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "Benchmark"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Resource\BenchmarkDialog\BenchmarkDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\Resource\BenchmarkDialog\BenchmarkDialog.h
# End Source File
# Begin Source File

SOURCE=.\Resource\BenchmarkDialog\resource.h
# End Source File
# Begin Source File

SOURCE=.\Resource\BenchmarkDialog\resource.rc
# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "Split"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Resource\SplitDialog\resource.h
# End Source File
# Begin Source File

SOURCE=.\Resource\SplitDialog\resource.rc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\Resource\SplitDialog\SplitDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\Resource\SplitDialog\SplitDialog.h
# End Source File
# End Group
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

SOURCE=..\..\Windows\Control\ComboBox.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Control\ComboBox.h
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Control\CommandBar.h
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Control\Dialog.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Control\Dialog.h
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Control\Edit.h
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Control\ImageList.h
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Control\ListView.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Control\ListView.h
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Control\ProgressBar.h
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Control\PropertyPage.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Control\PropertyPage.h
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Control\ReBar.h
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Control\Static.h
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Control\StatusBar.h
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Control\ToolBar.h
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Control\Trackbar.h
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Control\Window2.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Control\Window2.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\Windows\Defs.h
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Device.h
# End Source File
# Begin Source File

SOURCE=..\..\Windows\DLL.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Windows\DLL.h
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Error.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Error.h
# End Source File
# Begin Source File

SOURCE=..\..\Windows\FileDir.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Windows\FileDir.h
# End Source File
# Begin Source File

SOURCE=..\..\Windows\FileFind.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Windows\FileFind.h
# End Source File
# Begin Source File

SOURCE=..\..\Windows\FileIO.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Windows\FileIO.h
# End Source File
# Begin Source File

SOURCE=..\..\Windows\FileName.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Windows\FileName.h
# End Source File
# Begin Source File

SOURCE=..\..\Windows\FileSystem.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Windows\FileSystem.h
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Handle.h
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Menu.h
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Net.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Net.h
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Process.h
# End Source File
# Begin Source File

SOURCE=..\..\Windows\PropVariant.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Windows\PropVariant.h
# End Source File
# Begin Source File

SOURCE=..\..\Windows\PropVariantConversions.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Windows\PropVariantConversions.h
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Registry.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Registry.h
# End Source File
# Begin Source File

SOURCE=..\..\Windows\ResourceString.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Windows\ResourceString.h
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Shell.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Shell.h
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Synchronization.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Synchronization.h
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Thread.h
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Time.h
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Timer.h
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Window.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Windows\Window.h
# End Source File
# End Group
# Begin Group "Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Common\Alloc.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\Alloc.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\Buffer.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\ComTry.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\CRC.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\CRC.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\Defs.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\DynamicBuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\Exception.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\IntToString.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\IntToString.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\Lang.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\Lang.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\MyCom.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\NewHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\NewHandler.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\Random.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\Random.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\StdInStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\StdInStream.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\StdOutStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\StdOutStream.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\String.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\String.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\StringConvert.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\StringConvert.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\StringToInt.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\StringToInt.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\TextConfig.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\TextConfig.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\Types.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\UTFConvert.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\UTFConvert.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\Vector.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\Vector.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\Wildcard.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\Wildcard.h
# End Source File
# End Group
# End Group
# Begin Group "UI Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\UI\Common\ArchiveName.cpp
# End Source File
# Begin Source File

SOURCE=..\UI\Common\ArchiveName.h
# End Source File
# Begin Source File

SOURCE=..\UI\Common\CompressCall.cpp
# End Source File
# Begin Source File

SOURCE=..\UI\Common\CompressCall.h
# End Source File
# Begin Source File

SOURCE=..\UI\Common\PropIDUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\UI\Common\PropIDUtils.h
# End Source File
# End Group
# Begin Group "7-Zip Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Common\FilePathAutoRename.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\FilePathAutoRename.h
# End Source File
# Begin Source File

SOURCE=..\Common\FileStreams.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\FileStreams.h
# End Source File
# Begin Source File

SOURCE=..\Common\StreamObjects.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\StreamObjects.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\7zFM.exe.manifest
# End Source File
# Begin Source File

SOURCE=.\Add2.bmp
# End Source File
# Begin Source File

SOURCE=.\Copy2.bmp
# End Source File
# Begin Source File

SOURCE=.\Delete2.bmp
# End Source File
# Begin Source File

SOURCE=.\Extract2.bmp
# End Source File
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

SOURCE=.\Info.bmp
# End Source File
# Begin Source File

SOURCE=.\Info2.bmp
# End Source File
# Begin Source File

SOURCE=.\Move2.bmp
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

SOURCE=.\PluginInterface.h
# End Source File
# Begin Source File

SOURCE=.\PluginLoader.h
# End Source File
# Begin Source File

SOURCE=.\PropertyName.cpp
# End Source File
# Begin Source File

SOURCE=.\PropertyName.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
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

SOURCE=.\Test2.bmp
# End Source File
# Begin Source File

SOURCE=.\TextPairs.cpp
# End Source File
# Begin Source File

SOURCE=.\TextPairs.h
# End Source File
# End Target
# End Project
