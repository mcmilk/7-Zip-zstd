PROG = 7zG.exe
LIBS = $(LIBS) user32.lib advapi32.lib oleaut32.lib shell32.lib comctl32.lib htmlhelp.lib ole32.lib comdlg32.lib
CFLAGS = $(CFLAGS) -I ../../../ -DLANG

GUI_OBJS = \
  $O\CompressDialog.obj \
  $O\ExtractDialog.obj \
  $O\ExtractGUI.obj \
  $O\GUI.obj \
  $O\OpenCallbackGUI.obj \
  $O\UpdateCallbackGUI.obj \
  $O\UpdateGUI.obj \

COMMON_OBJS = \
  $O\Alloc.obj \
  $O\CommandLineParser.obj \
  $O\IntToString.obj \
  $O\Lang.obj \
  $O\ListFileUtils.obj \
  $O\NewHandler.obj \
  $O\StdInStream.obj \
  $O\String.obj \
  $O\StringConvert.obj \
  $O\StringToInt.obj \
  $O\TextConfig.obj \
  $O\UTFConvert.obj \
  $O\Vector.obj \
  $O\Wildcard.obj \

WIN_OBJS = \
  $O\DLL.obj \
  $O\Error.obj \
  $O\FileDir.obj \
  $O\FileFind.obj \
  $O\FileIO.obj \
  $O\FileName.obj \
  $O\PropVariant.obj \
  $O\PropVariantConversions.obj \
  $O\Registry.obj \
  $O\ResourceString.obj \
  $O\Shell.obj \
  $O\Synchronization.obj \
  $O\Window.obj \

WIN_CTRL_OBJS = \
  $O\ComboBox.obj \
  $O\Dialog.obj \
  $O\ListView.obj \

7ZIP_COMMON_OBJS = \
  $O\FilePathAutoRename.obj \
  $O\FileStreams.obj \

UI_COMMON_OBJS = \
  $O\ArchiveCommandLine.obj \
  $O\ArchiveExtractCallback.obj \
  $O\ArchiveOpenCallback.obj \
  $O\ArchiverInfo.obj \
  $O\DefaultName.obj \
  $O\EnumDirItems.obj \
  $O\Extract.obj \
  $O\ExtractingFilePath.obj \
  $O\OpenArchive.obj \
  $O\PropIDUtils.obj \
  $O\SortUtils.obj \
  $O\TempFiles.obj \
  $O\Update.obj \
  $O\UpdateAction.obj \
  $O\UpdateCallback.obj \
  $O\UpdatePair.obj \
  $O\UpdateProduce.obj \
  $O\WorkDir.obj \
  $O\ZipRegistry.obj \

FM_OBJS = \
  $O\ExtractCallback.obj \
  $O\FormatUtils.obj \
  $O\HelpUtils.obj \
  $O\LangUtils.obj \
  $O\OpenCallback.obj \
  $O\ProgramLocation.obj \
  $O\RegistryUtils.obj \
  $O\SplitUtils.obj \
  $O\StringUtils.obj \
  $O\UpdateCallback100.obj \

OBJS = \
  $O\StdAfx.obj \
  $(GUI_OBJS) \
  $(COMMON_OBJS) \
  $(WIN_OBJS) \
  $(WIN_CTRL_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(UI_COMMON_OBJS) \
  $(FM_OBJS)\
  $O\MyMessages.obj \
  $O\MessagesDialog.obj \
  $O\OverwriteDialog.obj \
  $O\PasswordDialog.obj \
  $O\ProgressDialog.obj \
  $O\CopyCoder.obj \
  $O\resource.res

!include "../../../Build.mak"

$(GUI_OBJS): $(*B).cpp
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
$(FM_OBJS): ../../FileManager/$(*B).cpp
	$(COMPL)
$O\MyMessages.obj: ../Explorer/MyMessages.cpp
	$(COMPL)
$O\MessagesDialog.obj: ../../FileManager/Resource/MessagesDialog/$(*B).cpp
	$(COMPL)
$O\OverwriteDialog.obj: ../../FileManager/Resource/OverwriteDialog./$(*B).cpp
	$(COMPL)
$O\PasswordDialog.obj: ../../FileManager/Resource/PasswordDialog/$(*B).cpp
	$(COMPL)
$O\ProgressDialog.obj: ../../FileManager/Resource/ProgressDialog2/$(*B).cpp
	$(COMPL)
$O\CopyCoder.obj: ../../Compress/Copy/$(*B).cpp
	$(COMPL)
