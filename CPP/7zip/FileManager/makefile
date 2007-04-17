PROG = 7zFM.exe
LIBS = $(LIBS) user32.lib oleaut32.lib advapi32.lib shell32.lib ole32.lib comctl32.lib htmlhelp.lib Mpr.lib Gdi32.lib comdlg32.lib
CFLAGS = $(CFLAGS) -I../../ \
  -DLANG \
  -DWIN_LONG_PATH \

FM_OBJS = \
  $O\App.obj \
  $O\ClassDefs.obj \
  $O\EnumFormatEtc.obj \
  $O\ExtractCallback.obj \
  $O\FileFolderPluginOpen.obj \
  $O\FilePlugins.obj \
  $O\FM.obj \
  $O\FormatUtils.obj \
  $O\FSDrives.obj \
  $O\FSFolder.obj \
  $O\FSFolderCopy.obj \
  $O\HelpUtils.obj \
  $O\LangUtils.obj \
  $O\MyLoadMenu.obj \
  $O\NetFolder.obj \
  $O\OpenCallback.obj \
  $O\OptionsDialog.obj \
  $O\Panel.obj \
  $O\PanelCopy.obj \
  $O\PanelCrc.obj \
  $O\PanelDrag.obj \
  $O\PanelFolderChange.obj \
  $O\PanelItemOpen.obj \
  $O\PanelItems.obj \
  $O\PanelKey.obj \
  $O\PanelListNotify.obj \
  $O\PanelMenu.obj \
  $O\PanelOperations.obj \
  $O\PanelSelect.obj \
  $O\PanelSort.obj \
  $O\PanelSplitFile.obj \
  $O\PhysDriveFolder.obj \
  $O\ProgramLocation.obj \
  $O\PropertyName.obj \
  $O\RegistryAssociations.obj \
  $O\RegistryPlugins.obj \
  $O\RegistryUtils.obj \
  $O\RootFolder.obj \
  $O\SplitUtils.obj \
  $O\StringUtils.obj \
  $O\SysIconUtils.obj \
  $O\TextPairs.obj \
  $O\UpdateCallback100.obj \
  $O\ViewSettings.obj \

COMMON_OBJS = \
  $O\CRC.obj \
  $O\IntToString.obj \
  $O\Lang.obj \
  $O\ListFileUtils.obj \
  $O\NewHandler.obj \
  $O\Random.obj \
  $O\StdInStream.obj \
  $O\StdOutStream.obj \
  $O\String.obj \
  $O\StringConvert.obj \
  $O\StringToInt.obj \
  $O\TextConfig.obj \
  $O\UTFConvert.obj \
  $O\Vector.obj \
  $O\Wildcard.obj \

WIN_OBJS = \
  $O\CommonDialog.obj \
  $O\DLL.obj \
  $O\Error.obj \
  $O\FileDevice.obj \
  $O\FileDir.obj \
  $O\FileFind.obj \
  $O\FileIO.obj \
  $O\FileName.obj \
  $O\FileSystem.obj \
  $O\Memory.obj \
  $O\MemoryLock.obj \
  $O\Menu.obj \
  $O\Net.obj \
  $O\PropVariant.obj \
  $O\PropVariantConversions.obj \
  $O\Registry.obj \
  $O\ResourceString.obj \
  $O\Security.obj \
  $O\Shell.obj \
  $O\Synchronization.obj \
  $O\Window.obj \

WIN_CTRL_OBJS = \
  $O\ComboBox.obj \
  $O\Dialog.obj \
  $O\ListView.obj \
  $O\PropertyPage.obj \
  $O\Window2.obj \

7ZIP_COMMON_OBJS = \
  $O\FilePathAutoRename.obj \
  $O\FileStreams.obj \
  $O\StreamObjects.obj \

UI_COMMON_OBJS = \
  $O\ArchiveName.obj \
  $O\CompressCall.obj \
  $O\PropIDUtils.obj \

C_OBJS = \
  $O\Sort.obj \
  $O\Alloc.obj \

!include "../Crc2.mak"

OBJS = \
  $O\StdAfx.obj \
  $(FM_OBJS)\
  $(COMMON_OBJS) \
  $(WIN_OBJS) \
  $(WIN_CTRL_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(UI_COMMON_OBJS) \
  $O\AboutDialog.obj \
  $O\ComboDialog.obj \
  $O\CopyDialog.obj \
  $O\EditPage.obj \
  $O\LangPage.obj \
  $O\ListViewDialog.obj \
  $O\MessagesDialog.obj \
  $O\OverwriteDialog.obj \
  $O\PasswordDialog.obj \
  $O\PluginsPage.obj \
  $O\ProgressDialog.obj \
  $O\SettingsPage.obj \
  $O\SplitDialog.obj \
  $O\SystemPage.obj \
  $(CRC_OBJS) \
  $(C_OBJS) \
  $O\resource.res \

!include "../../Build.mak"

$(FM_OBJS): $(*B).cpp
	$(COMPL)
$(COMMON_OBJS): ../../Common/$(*B).cpp
	$(COMPL)
$(WIN_OBJS): ../../Windows/$(*B).cpp
	$(COMPL)
$(WIN_CTRL_OBJS): ../../Windows/Control/$(*B).cpp
	$(COMPL)
$(7ZIP_COMMON_OBJS): ../Common/$(*B).cpp
	$(COMPL)
$(UI_COMMON_OBJS): ../UI/Common/$(*B).cpp
	$(COMPL)

$O\AboutDialog.obj: Resource/AboutDialog/AboutDialog.cpp
	$(COMPL)
$O\ComboDialog.obj: Resource/ComboDialog/ComboDialog.cpp
	$(COMPL)
$O\CopyDialog.obj: Resource/CopyDialog/CopyDialog.cpp
	$(COMPL)
$O\EditPage.obj: Resource/EditPage/EditPage.cpp
	$(COMPL)
$O\LangPage.obj: Resource/LangPage/LangPage.cpp
	$(COMPL)
$O\ListViewDialog.obj: Resource/ListViewDialog/ListViewDialog.cpp
	$(COMPL)
$O\MessagesDialog.obj: Resource/MessagesDialog/MessagesDialog.cpp
	$(COMPL)
$O\OverwriteDialog.obj: Resource/OverwriteDialog/OverwriteDialog.cpp
	$(COMPL)
$O\PasswordDialog.obj: Resource/PasswordDialog/PasswordDialog.cpp
	$(COMPL)
$O\PluginsPage.obj: Resource/PluginsPage/PluginsPage.cpp
	$(COMPL)
$O\ProgressDialog.obj: Resource/ProgressDialog2/ProgressDialog.cpp
	$(COMPL)
$O\SettingsPage.obj: Resource/SettingsPage/SettingsPage.cpp
	$(COMPL)
$O\SplitDialog.obj: Resource/SplitDialog/SplitDialog.cpp
	$(COMPL)
$O\SystemPage.obj: Resource/SystemPage/SystemPage.cpp
	$(COMPL)
$(C_OBJS): ../../../C/$(*B).c
	$(COMPL_O2)


!IF "$(CPU)" != "IA64"
!IF "$(CPU)" != "AMD64"
$O\7zCrcT8U.obj: ../../../Asm/x86/$(*B).asm
	$(COMPL_ASM)
!ELSE
$O\7zCrcT8U.obj: ../../../Asm/x64/$(*B).asm
	$(COMPL_ASM)
!ENDIF
$O\7zCrcT8.obj: ../../../C/$(*B).c
	$(COMPL_O2)
!ELSE
$(CRC_OBJS): ../../../C/$(*B).c
	$(COMPL_O2)
!ENDIF


