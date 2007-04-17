PROG = 7-zip.dll
DEF_FILE = Explorer.def
LIBS = $(LIBS) user32.lib oleaut32.lib advapi32.lib htmlhelp.lib shell32.lib comctl32.lib ole32.lib comdlg32.lib
CFLAGS = $(CFLAGS) -I ../../../ \
  -DLANG \
  -DNEW_FOLDER_INTERFACE \
  -DWIN_LONG_PATH \
  -DEXTERNAL_CODECS \

EXPLORER_OBJS = \
  $O\DllExports.obj \
  $O\ContextMenu.obj \
  $O\MyMessages.obj \
  $O\OptionsDialog.obj \
  $O\RegistryContextMenu.obj \

COMMON_OBJS = \
  $O\IntToString.obj \
  $O\Lang.obj \
  $O\ListFileUtils.obj \
  $O\NewHandler.obj \
  $O\Random.obj \
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
  $O\Menu.obj \
  $O\PropVariant.obj \
  $O\PropVariantConversions.obj \
  $O\Registry.obj \
  $O\ResourceString.obj \
  $O\Shell.obj \
  $O\Synchronization.obj \
  $O\Window.obj \

WIN_CTRL_OBJS = \
  $O\Dialog.obj \
  $O\PropertyPage.obj \

7ZIP_COMMON_OBJS = \
  $O\FilePathAutoRename.obj \
  $O\FileStreams.obj \
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


FM_COMMON_OBJS = \
  $O\FormatUtils.obj \
  $O\HelpUtils.obj \
  $O\LangUtils.obj \
  $O\ProgramLocation.obj \
  $O\RegistryUtils.obj \

C_OBJS = \
  $O\Alloc.obj \
  $O\Sort.obj \

OBJS = \
  $O\StdAfx.obj \
  $(EXPLORER_OBJS) \
  $(COMMON_OBJS) \
  $(WIN_OBJS) \
  $(WIN_CTRL_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(UI_COMMON_OBJS) \
  $(AGENT_OBJS) \
  $(FM_COMMON_OBJS)\
  $(C_OBJS) \
  $O\SystemPage.obj \
  $O\FoldersPage.obj \
  $O\CopyCoder.obj \
  $O\resource.res

!include "../../../Build.mak"

$(EXPLORER_OBJS): $(*B).cpp
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
$(FM_COMMON_OBJS): ../../FileManager/$(*B).cpp
	$(COMPL)
$O\SystemPage.obj: SystemPage/$(*B).cpp
	$(COMPL)
$O\FoldersPage.obj: FoldersPage/$(*B).cpp
	$(COMPL)
$O\CopyCoder.obj: ../../Compress/Copy/$(*B).cpp
	$(COMPL)
$(C_OBJS): ../../../../C/$(*B).c
	$(COMPL_O2)
