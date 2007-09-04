PROG = 7zFM.exe
LIBS = $(LIBS) user32.lib oleaut32.lib advapi32.lib shell32.lib ole32.lib comctl32.lib htmlhelp.lib Mpr.lib Gdi32.lib comdlg32.lib
CFLAGS = $(CFLAGS) -I ../../../ \
  -DLANG \
  -DWIN_LONG_PATH \
  -DNEW_FOLDER_INTERFACE \
  -DEXTERNAL_CODECS \

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
  $O\ProgressDialog2.obj \
  $O\SettingsPage.obj \
  $O\SplitDialog.obj \
  $O\SystemPage.obj \

COMMON_OBJS = \
  $O\CRC.obj \
  $O\IntToString.obj \
  $O\Lang.obj \
  $O\MyString.obj \
  $O\MyVector.obj \
  $O\NewHandler.obj \
  $O\Random.obj \
  $O\StringConvert.obj \
  $O\StringToInt.obj \
  $O\TextConfig.obj \
  $O\UTFConvert.obj \
  $O\Wildcard.obj \

WIN_OBJS = \
  $O\Clipboard.obj \
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
  $O\ProgressUtils.obj \
  $O\StreamObjects.obj \
  $O\StreamUtils.obj \

UI_COMMON_OBJS = \
  $O\ArchiveExtractCallback.obj \
  $O\ArchiveName.obj \
  $O\ArchiveOpenCallback.obj \
  $O\CompressCall.obj \
  $O\DefaultName.obj \
  $O\EnumDirItems.obj \
  $O\ExtractingFilePath.obj \
  $O\LoadCodecs.obj \
  $O\OpenArchive.obj \
  $O\PropIDUtils.obj \
  $O\SortUtils.obj \
  $O\UpdateAction.obj \
  $O\UpdateCallback.obj \
  $O\UpdatePair.obj \
  $O\UpdateProduce.obj \
  $O\WorkDir.obj \
  $O\ZipRegistry.obj \

AGENT_OBJS = \
  $O\Agent.obj \
  $O\AgentOut.obj \
  $O\AgentProxy.obj \
  $O\ArchiveFolder.obj \
  $O\ArchiveFolderOpen.obj \
  $O\ArchiveFolderOut.obj \
  $O\UpdateCallbackAgent.obj \

C_OBJS = \
  $O\Alloc.obj \
  $O\Sort.obj \
  $O\Threads.obj \

!include "../../Crc2.mak"

OBJS = \
  $O\StdAfx.obj \
  $(FM_OBJS)\
  $(COMMON_OBJS) \
  $(WIN_OBJS) \
  $(WIN_CTRL_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(UI_COMMON_OBJS) \
  $(AGENT_OBJS) \
  $O\CopyCoder.obj \
  $(CRC_OBJS) \
  $(C_OBJS) \
  $O\resource.res \

!include "../../../Build.mak"

$(FM_OBJS): $(*B).cpp
	$(COMPL)
$(COMMON_OBJS): ../../../Common/$(*B).cpp
	$(COMPL)
$(WIN_OBJS): ../../../Windows/$(*B).cpp
	$(COMPL)
$(WIN_CTRL_OBJS): ../../../Windows/Control/$(*B).cpp
	$(COMPL)
$(7ZIP_COMMON_OBJS): ../../Common/$(*B).cpp
	$(COMPL)
$(UI_COMMON_OBJS): ../Common/$(*B).cpp
	$(COMPL)
$(AGENT_OBJS): ../Agent/$(*B).cpp
	$(COMPL)

$O\CopyCoder.obj: ../../Compress/Copy/$(*B).cpp
	$(COMPL)
$(C_OBJS): ../../../../C/$(*B).c
	$(COMPL_O2)

!include "../../Crc.mak"
