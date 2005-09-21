PROG = 7z.sfx
LIBS = $(LIBS) user32.lib oleaut32.lib shell32.lib ole32.lib
CFLAGS = $(CFLAGS) -I ../../../ \
  -DEXCLUDE_COM \
  -DNO_REGISTRY \
  -DEXTRACT_ONLY \
  -D_SFX \
  -DFORMAT_7Z \
  -DCOMPRESS_BCJ_X86 \
  -DCOMPRESS_BCJ2 \
  -DCOMPRESS_COPY \
  -DCOMPRESS_LZMA \
  -DCOMPRESS_PPMD \
  -DCRYPTO_AES \
  -DCRYPTO_7ZAES \


SFX_WIN_OBJS = \
  $O\Main.obj \

GUI_OBJS = \
  $O\ExtractDialog.obj \
  $O\ExtractGUI.obj \
  $O\OpenCallbackGUI.obj \

COMMON_OBJS = \
  $O\Alloc.obj \
  $O\CommandLineParser.obj \
  $O\CRC.obj \
  $O\IntToString.obj \
  $O\NewHandler.obj \
  $O\String.obj \
  $O\StringConvert.obj \
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
  $O\ResourceString.obj \
  $O\Shell.obj \
  $O\Synchronization.obj \
  $O\Window.obj \

WIN_CTRL_OBJS = \
  $O\Dialog.obj \

7ZIP_COMMON_OBJS = \
  $O\FilePathAutoRename.obj \
  $O\FileStreams.obj \
  $O\InBuffer.obj \
  $O\LimitedStreams.obj \
  $O\LockedStream.obj \
  $O\OutBuffer.obj \
  $O\ProgressUtils.obj \
  $O\StreamBinder.obj \
  $O\StreamObjects.obj \
  $O\StreamUtils.obj \

UI_COMMON_OBJS = \
  $O\ArchiveExtractCallback.obj \
  $O\ArchiveOpenCallback.obj \
  $O\ArchiverInfo.obj \
  $O\DefaultName.obj \
  $O\Extract.obj \
  $O\ExtractingFilePath.obj \
  $O\OpenArchive.obj \

FM_OBJS = \
  $O\ExtractCallback.obj \
  $O\FormatUtils.obj \

AR_COMMON_OBJS = \
  $O\CoderMixer2.obj \
  $O\CoderMixer2MT.obj \
  $O\CrossThreadProgress.obj \
  $O\FilterCoder.obj \
  $O\ItemNameUtils.obj \
  $O\OutStreamWithCRC.obj \

7Z_OBJS = \
  $O\7zDecode.obj \
  $O\7zExtract.obj \
  $O\7zFolderOutStream.obj \
  $O\7zHandler.obj \
  $O\7zHeader.obj \
  $O\7zIn.obj \
  $O\7zMethodID.obj \

BRANCH_OPT_OBJS = \
  $O\BranchCoder.obj \
  $O\x86.obj \
  $O\x86_2.obj \

LZ_OBJS = \
  $O\LZOutWindow.obj \

LZMA_OPT_OBJS = \
  $O\LZMADecoder.obj \

PPMD_OPT_OBJS = \
  $O\PPMDDecoder.obj \

7ZAES_OPT_OBJS = \
  $O\7zAES.obj \
  $O\SHA256.obj \

AES_OPT_OBJS = \
  $O\MyAES.obj \

AES_ORIG_OBJS = \
  $O\aescrypt.obj \
  $O\aeskey.obj \
  $O\aestab.obj \

OBJS = \
  $O\StdAfx.obj \
  $(SFX_WIN_OBJS) \
  $(GUI_OBJS) \
  $(COMMON_OBJS) \
  $(WIN_OBJS) \
  $(WIN_CTRL_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(UI_COMMON_OBJS) \
  $(FM_OBJS)\
  $(AR_COMMON_OBJS) \
  $(7Z_OBJS) \
  $(BRANCH_OPT_OBJS) \
  $(LZ_OBJS) \
  $(LZMA_OPT_OBJS) \
  $(PPMD_OPT_OBJS) \
  $O\CopyCoder.obj \
  $(7ZAES_OPT_OBJS) \
  $(AES_OPT_OBJS) \
  $(AES_ORIG_OBJS) \
  $O\MyMessages.obj \
  $O\MessagesDialog.obj \
  $O\OverwriteDialog.obj \
  $O\PasswordDialog.obj \
  $O\ProgressDialog.obj \
  $O\resource.res


!include "../../../Build.mak"

$(SFX_WIN_OBJS): $(*B).cpp
	$(COMPL)

$(GUI_OBJS): ../../UI/GUI/$(*B).cpp
	$(COMPL)
$(COMMON_OBJS): ../../../Common/$(*B).cpp
	$(COMPL)
$(WIN_OBJS): ../../../Windows/$(*B).cpp
	$(COMPL)
$(WIN_CTRL_OBJS): ../../../Windows/Control/$(*B).cpp
	$(COMPL)
$(7ZIP_COMMON_OBJS): ../../Common/$(*B).cpp
	$(COMPL)
$(UI_COMMON_OBJS): ../../UI/Common/$(*B).cpp
	$(COMPL)
$(FM_OBJS): ../../FileManager/$(*B).cpp
	$(COMPL)
$(AR_COMMON_OBJS): ../../Archive/Common/$(*B).cpp
	$(COMPL)

$(7Z_OBJS): ../../Archive/7z/$(*B).cpp
	$(COMPL)
$(BRANCH_OPT_OBJS): ../../Compress/Branch/$(*B).cpp
	$(COMPL)
$(LZ_OBJS): ../../Compress/LZ/$(*B).cpp
	$(COMPL)
$(LZMA_OPT_OBJS): ../../Compress/LZMA/$(*B).cpp
	$(COMPL)
$(PPMD_OPT_OBJS): ../../Compress/PPMD/$(*B).cpp
	$(COMPL)

$O\CopyCoder.obj: ../../Compress/Copy/$(*B).cpp
	$(COMPL)

$(7ZAES_OPT_OBJS): ../../Crypto/7zAES/$(*B).cpp
	$(COMPL)
$(AES_OPT_OBJS): ../../Crypto/AES/$(*B).cpp
	$(COMPL)
$(AES_ORIG_OBJS): ../../Crypto/AES/$(*B).c
	$(COMPL_O1)

$O\MyMessages.obj: ../../UI/Explorer/MyMessages.cpp
	$(COMPL)
$O\MessagesDialog.obj: ../../FileManager/Resource/MessagesDialog/$(*B).cpp
	$(COMPL)
$O\OverwriteDialog.obj: ../../FileManager/Resource/OverwriteDialog./$(*B).cpp
	$(COMPL)
$O\PasswordDialog.obj: ../../FileManager/Resource/PasswordDialog/$(*B).cpp
	$(COMPL)
$O\ProgressDialog.obj: ../../FileManager/Resource/ProgressDialog/$(*B).cpp
	$(COMPL)
