PROG = 7z.sfx
LIBS = $(LIBS) user32.lib oleaut32.lib shell32.lib ole32.lib
CFLAGS = $(CFLAGS) -I ../../../ \
  -DNO_REGISTRY \
  -DEXTRACT_ONLY \
  -DNO_READ_FROM_CODER \
  -D_SFX \

SFX_WIN_OBJS = \
  $O\Main.obj \

GUI_OBJS = \
  $O\ExtractDialog.obj \
  $O\ExtractGUI.obj \
  $O\OpenCallbackGUI.obj \

COMMON_OBJS = \
  $O\CRC.obj \
  $O\CommandLineParser.obj \
  $O\IntToString.obj \
  $O\NewHandler.obj \
  $O\MyString.obj \
  $O\StringConvert.obj \
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
  $O\ResourceString.obj \
  $O\Shell.obj \
  $O\Synchronization.obj \
  $O\Window.obj \

WIN_CTRL_OBJS = \
  $O\Dialog.obj \

7ZIP_COMMON_OBJS = \
  $O\CreateCoder.obj \
  $O\FilePathAutoRename.obj \
  $O\FileStreams.obj \
  $O\InBuffer.obj \
  $O\FilterCoder.obj \
  $O\LimitedStreams.obj \
  $O\LockedStream.obj \
  $O\OutBuffer.obj \
  $O\ProgressUtils.obj \
  $O\StreamBinder.obj \
  $O\StreamObjects.obj \
  $O\StreamUtils.obj \
  $O\VirtThread.obj \

UI_COMMON_OBJS = \
  $O\ArchiveExtractCallback.obj \
  $O\ArchiveOpenCallback.obj \
  $O\DefaultName.obj \
  $O\Extract.obj \
  $O\ExtractingFilePath.obj \
  $O\LoadCodecs.obj \
  $O\OpenArchive.obj \

FM_OBJS = \
  $O\ExtractCallback.obj \
  $O\FormatUtils.obj \
  $O\MessagesDialog.obj \
  $O\OverwriteDialog.obj \
  $O\PasswordDialog.obj \
  $O\ProgressDialog.obj \

AR_COMMON_OBJS = \
  $O\CoderMixer2.obj \
  $O\CoderMixer2MT.obj \
  $O\CrossThreadProgress.obj \
  $O\ItemNameUtils.obj \
  $O\OutStreamWithCRC.obj \

7Z_OBJS = \
  $O\7zDecode.obj \
  $O\7zExtract.obj \
  $O\7zFolderOutStream.obj \
  $O\7zHandler.obj \
  $O\7zHeader.obj \
  $O\7zIn.obj \
  $O\7zRegister.obj \

BRANCH_OPT_OBJS = \
  $O\BranchCoder.obj \
  $O\x86.obj \
  $O\x86_2.obj \
  $O\BCJRegister.obj \
  $O\BCJ2Register.obj \

COPY_OBJS = \
  $O\CopyCoder.obj \
  $O\CopyRegister.obj \

LZ_OBJS = \
  $O\LZOutWindow.obj \

LZMA_OPT_OBJS = \
  $O\LZMADecoder.obj \
  $O\LZMARegister.obj \

PPMD_OPT_OBJS = \
  $O\PPMDDecoder.obj \
  $O\PPMDRegister.obj \

7ZAES_OPT_OBJS = \
  $O\7zAES.obj \
  $O\7zAESRegister.obj \

AES_OPT_OBJS = \
  $O\MyAES.obj \

CRYPTO_HASH_OBJS = \
  $O\Sha256.obj \

C_OBJS = \
  $O\Alloc.obj \
  $O\Threads.obj \

C_BRANCH_OBJS = \
  $O\BranchX86.obj \

C_CRYPTO = \
  $O\Aes.obj \

!include "../../Crc2.mak"

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
  $(COPY_OBJS) \
  $(LZ_OBJS) \
  $(LZMA_OPT_OBJS) \
  $(PPMD_OPT_OBJS) \
  $(CRYPTO_HASH_OBJS) \
  $(7ZAES_OPT_OBJS) \
  $(AES_OPT_OBJS) \
  $O\MyMessages.obj \
  $(C_OBJS) \
  $(C_BRANCH_OBJS) \
  $(CRC_OBJS) \
  $(C_CRYPTO) \
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
$(FM_OBJS): ../../UI/FileManager/$(*B).cpp
	$(COMPL)
$(AR_COMMON_OBJS): ../../Archive/Common/$(*B).cpp
	$(COMPL)

$(7Z_OBJS): ../../Archive/7z/$(*B).cpp
	$(COMPL)
$(BRANCH_OPT_OBJS): ../../Compress/Branch/$(*B).cpp
	$(COMPL)
$(COPY_OBJS): ../../Compress/Copy/$(*B).cpp
	$(COMPL)
$(LZ_OBJS): ../../Compress/LZ/$(*B).cpp
	$(COMPL)
$(LZMA_OPT_OBJS): ../../Compress/LZMA/$(*B).cpp
	$(COMPL)
$(PPMD_OPT_OBJS): ../../Compress/PPMD/$(*B).cpp
	$(COMPL)

$(CRYPTO_HASH_OBJS): ../../Crypto/Hash/$(*B).cpp
	$(COMPL)
$(7ZAES_OPT_OBJS): ../../Crypto/7zAES/$(*B).cpp
	$(COMPL)
$(AES_OPT_OBJS): ../../Crypto/AES/$(*B).cpp
	$(COMPL)

$O\MyMessages.obj: ../../UI/Explorer/MyMessages.cpp
	$(COMPL)
$(C_OBJS): ../../../../C/$(*B).c
	$(COMPL_O1)
$(C_BRANCH_OBJS): ../../../../C/Compress/Branch/$(*B).c
	$(COMPL_O1)
$(C_CRYPTO): ../../../../C/Crypto/$(*B).c
	$(COMPL_O1)

!include "../../Crc.mak"
