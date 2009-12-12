PROG = 7zG.exe
CFLAGS = $(CFLAGS) -I ../../../  \
  -DLANG \
  -DEXTERNAL_CODECS \

!IFDEF UNDER_CE
LIBS = $(LIBS) ceshell.lib Commctrl.lib
!ELSE
LIBS = $(LIBS) comctl32.lib htmlhelp.lib comdlg32.lib
CFLAGS = $(CFLAGS) -DWIN_LONG_PATH -DSUPPORT_DEVICE_FILE -D_7ZIP_LARGE_PAGES
!ENDIF

GUI_OBJS = \
  $O\BenchmarkDialog.obj \
  $O\CompressDialog.obj \
  $O\ExtractDialog.obj \
  $O\ExtractGUI.obj \
  $O\GUI.obj \
  $O\UpdateCallbackGUI.obj \
  $O\UpdateGUI.obj \

COMMON_OBJS = \
  $O\CommandLineParser.obj \
  $O\CRC.obj \
  $O\IntToString.obj \
  $O\Lang.obj \
  $O\ListFileUtils.obj \
  $O\MyString.obj \
  $O\MyVector.obj \
  $O\NewHandler.obj \
  $O\StringConvert.obj \
  $O\StringToInt.obj \
  $O\TextConfig.obj \
  $O\UTFConvert.obj \
  $O\Wildcard.obj \

WIN_OBJS = \
  $O\CommonDialog.obj \
  $O\DLL.obj \
  $O\Error.obj \
  $O\FileDir.obj \
  $O\FileFind.obj \
  $O\FileIO.obj \
  $O\FileName.obj \
  $O\MemoryLock.obj \
  $O\PropVariant.obj \
  $O\PropVariantConversions.obj \
  $O\Registry.obj \
  $O\ResourceString.obj \
  $O\Shell.obj \
  $O\Synchronization.obj \
  $O\System.obj \
  $O\Time.obj \
  $O\Window.obj \

WIN_CTRL_OBJS = \
  $O\ComboBox.obj \
  $O\Dialog.obj \
  $O\ListView.obj \

7ZIP_COMMON_OBJS = \
  $O\CreateCoder.obj \
  $O\FilePathAutoRename.obj \
  $O\FileStreams.obj \
  $O\FilterCoder.obj \
  $O\ProgressUtils.obj \
  $O\StreamUtils.obj \

UI_COMMON_OBJS = \
  $O\ArchiveCommandLine.obj \
  $O\ArchiveExtractCallback.obj \
  $O\ArchiveOpenCallback.obj \
  $O\Bench.obj \
  $O\DefaultName.obj \
  $O\EnumDirItems.obj \
  $O\Extract.obj \
  $O\ExtractingFilePath.obj \
  $O\LoadCodecs.obj \
  $O\OpenArchive.obj \
  $O\PropIDUtils.obj \
  $O\SetProperties.obj \
  $O\SortUtils.obj \
  $O\TempFiles.obj \
  $O\Update.obj \
  $O\UpdateAction.obj \
  $O\UpdateCallback.obj \
  $O\UpdatePair.obj \
  $O\UpdateProduce.obj \
  $O\WorkDir.obj \
  $O\ZipRegistry.obj \

AR_COMMON_OBJS = \
  $O\OutStreamWithCRC.obj \

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
  $O\OverwriteDialog.obj \
  $O\PasswordDialog.obj \
  $O\ProgressDialog2.obj \

!IFDEF UNDER_CE
FM_OBJS = $(FM_OBJS) \
  $O\BrowseDialog.obj \
  $O\SysIconUtils.obj \
!ENDIF

C_OBJS = \
  $O\Alloc.obj \
  $O\CpuArch.obj \
  $O\Threads.obj \

!include "../../Crc.mak"

OBJS = \
  $O\StdAfx.obj \
  $(GUI_OBJS) \
  $(COMMON_OBJS) \
  $(WIN_OBJS) \
  $(WIN_CTRL_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(UI_COMMON_OBJS) \
  $(AR_COMMON_OBJS) \
  $(FM_OBJS)\
  $O\MyMessages.obj \
  $O\CopyCoder.obj \
  $(LZMA_BENCH_OBJS) \
  $(C_OBJS) \
  $(ASM_OBJS) \
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
$(AR_COMMON_OBJS): ../../Archive/Common/$(*B).cpp
	$(COMPL)
$(FM_OBJS): ../FileManager/$(*B).cpp
	$(COMPL)
$O\MyMessages.obj: ../Explorer/MyMessages.cpp
	$(COMPL)

$O\CopyCoder.obj: ../../Compress/$(*B).cpp
	$(COMPL)
$(C_OBJS): ../../../../C/$(*B).c
	$(COMPL_O2)

!include "../../Asm.mak"
