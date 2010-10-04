PROG = 7-ZipFar.dll
DEF_FILE = Far.def
CFLAGS = $(CFLAGS) -I ../../../ \
  -DEXTERNAL_CODECS

!IFNDEF UNDER_CE
CFLAGS = $(CFLAGS) -DWIN_LONG_PATH
!ENDIF

FAR_OBJS = \
  $O\ExtractEngine.obj \
  $O\FarUtils.obj \
  $O\Main.obj \
  $O\OverwriteDialog.obj \
  $O\Plugin.obj \
  $O\PluginCommon.obj \
  $O\PluginDelete.obj \
  $O\PluginRead.obj \
  $O\PluginWrite.obj \
  $O\ProgressBox.obj \
  $O\UpdateCallback100.obj \

COMMON_OBJS = \
  $O\IntToString.obj \
  $O\NewHandler.obj \
  $O\MyString.obj \
  $O\StringConvert.obj \
  $O\StringToInt.obj \
  $O\MyVector.obj \
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
  $O\Synchronization.obj \
  $O\Time.obj \

7ZIP_COMMON_OBJS = \
  $O\FilePathAutoRename.obj \
  $O\FileStreams.obj \
  $O\ProgressUtils.obj \
  $O\StreamUtils.obj \

UI_COMMON_OBJS = \
  $O\ArchiveExtractCallback.obj \
  $O\ArchiveOpenCallback.obj \
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

AR_COMMON_OBJS = \
  $O\OutStreamWithCRC.obj \

AGENT_OBJS = \
  $O\Agent.obj \
  $O\AgentOut.obj \
  $O\AgentProxy.obj \
  $O\UpdateCallbackAgent.obj \

C_OBJS = \
  $O\Alloc.obj \
  $O\CpuArch.obj \
  $O\Sort.obj \
  $O\Threads.obj \

!include "../../Crc.mak"

OBJS = \
  $O\StdAfx.obj \
  $(FAR_OBJS) \
  $(COMMON_OBJS) \
  $(WIN_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(UI_COMMON_OBJS) \
  $(AR_COMMON_OBJS) \
  $(AGENT_OBJS) \
  $O\CopyCoder.obj \
  $(C_OBJS) \
  $(ASM_OBJS) \
  $O\resource.res

!include "../../../Build.mak"

$(FAR_OBJS): $(*B).cpp
	$(COMPL)
$(COMMON_OBJS): ../../../Common/$(*B).cpp
	$(COMPL)
$(WIN_OBJS): ../../../Windows/$(*B).cpp
	$(COMPL)
$(7ZIP_COMMON_OBJS): ../../Common/$(*B).cpp
	$(COMPL)
$(UI_COMMON_OBJS): ../Common/$(*B).cpp
	$(COMPL)
$(AR_COMMON_OBJS): ../../Archive/Common/$(*B).cpp
	$(COMPL)
$(AGENT_OBJS): ../Agent/$(*B).cpp
	$(COMPL)
$O\CopyCoder.obj: ../../Compress/$(*B).cpp
	$(COMPL)
$(C_OBJS): ../../../../C/$(*B).c
	$(COMPL_O2)
!include "../../Asm.mak"
