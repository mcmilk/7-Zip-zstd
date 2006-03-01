PROG = 7z.dll
DEF_FILE = ../Archive.def
CFLAGS = $(CFLAGS) -I ../../../ -DCOMPRESS_MT
LIBS = $(LIBS) oleaut32.lib user32.lib

7Z_OBJS = \
  $O\7zCompressionMode.obj \
  $O\7zDecode.obj \
  $O\7zEncode.obj \
  $O\7zExtract.obj \
  $O\7zFolderInStream.obj \
  $O\7zFolderOutStream.obj \
  $O\7zHandler.obj \
  $O\7zHandlerOut.obj \
  $O\7zHeader.obj \
  $O\7zIn.obj \
  $O\7zMethodID.obj \
  $O\7zMethods.obj \
  $O\7zOut.obj \
  $O\7zProperties.obj \
  $O\7zSpecStream.obj \
  $O\7zUpdate.obj \
  $O\DllExports.obj \

COMMON_OBJS = \
  $O\Alloc.obj \
  $O\CRC.obj \
  $O\IntToString.obj \
  $O\NewHandler.obj \
  $O\String.obj \
  $O\StringConvert.obj \
  $O\StringToInt.obj \
  $O\Vector.obj \

WIN_OBJS = \
  $O\DLL.obj \
  $O\FileDir.obj \
  $O\FileFind.obj \
  $O\FileIO.obj \
  $O\PropVariant.obj \
  $O\Synchronization.obj

7ZIP_COMMON_OBJS = \
  $O\InOutTempBuffer.obj \
  $O\LimitedStreams.obj \
  $O\LockedStream.obj \
  $O\OutBuffer.obj \
  $O\ProgressUtils.obj \
  $O\StreamBinder.obj \
  $O\StreamObjects.obj \
  $O\StreamUtils.obj \

AR_COMMON_OBJS = \
  $O\CodecsPath.obj \
  $O\CoderLoader.obj \
  $O\CoderMixer2.obj \
  $O\CoderMixer2MT.obj \
  $O\CrossThreadProgress.obj \
  $O\FilterCoder.obj \
  $O\InStreamWithCRC.obj \
  $O\ItemNameUtils.obj \
  $O\MultiStream.obj \
  $O\OutStreamWithCRC.obj \
  $O\ParseProperties.obj \

OBJS = \
  $O\StdAfx.obj \
  $(7Z_OBJS) \
  $(COMMON_OBJS) \
  $(WIN_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(AR_COMMON_OBJS) \
  $O\CopyCoder.obj \
  $O\resource.res

!include "../../../Build.mak"

$(7Z_OBJS): $(*B).cpp
	$(COMPL)
$(COMMON_OBJS): ../../../Common/$(*B).cpp
	$(COMPL)
$(WIN_OBJS): ../../../Windows/$(*B).cpp
	$(COMPL)
$(7ZIP_COMMON_OBJS): ../../Common/$(*B).cpp
	$(COMPL)
$(AR_COMMON_OBJS): ../Common/$(*B).cpp
	$(COMPL)
$O\CopyCoder.obj: ../../Compress/Copy/$(*B).cpp
	$(COMPL)
