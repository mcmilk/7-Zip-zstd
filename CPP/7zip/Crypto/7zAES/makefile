PROG = 7zAES.dll
DEF_FILE = ../Codec.def
CFLAGS = $(CFLAGS) -I ../../../
LIBS = $(LIBS) oleaut32.lib user32.lib

COMPRESS_OBJS = \
  $O\CodecExports.obj \
  $O\DllExports.obj \

7ZAES_OPT_OBJS = \
  $O\7zAES.obj \
  $O\7zAESRegister.obj \

AES_OPT_OBJS = \
  $O\MyAES.obj \

AES_ORIG_OBJS = \
  $O\aescrypt.obj \
  $O\aeskey.obj \
  $O\aestab.obj \

CRYPTO_HASH_OBJS = \
  $O\Sha256.obj \

COMMON_OBJS = \
  $O\NewHandler.obj \
  $O\StringConvert.obj \
  $O\Vector.obj \

WIN_OBJS = \
  $O\Synchronization.obj

7ZIP_COMMON_OBJS = \
  $O\StreamObjects.obj \
  $O\StreamUtils.obj \

C_OBJS = \
  $O\Alloc.obj \

OBJS = \
  $O\StdAfx.obj \
  $(COMPRESS_OBJS) \
  $(7ZAES_OPT_OBJS) \
  $(AES_OPT_OBJS) \
  $(AES_ORIG_OBJS) \
  $(CRYPTO_HASH_OBJS) \
  $(COMMON_OBJS) \
  $(WIN_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(C_OBJS) \
  $O\resource.res

!include "../../../Build.mak"

$(COMPRESS_OBJS): ../../Compress/$(*B).cpp
	$(COMPL)
$(AES_OPT_OBJS): ../AES/$(*B).cpp
	$(COMPL_O2)
$(AES_ORIG_OBJS): ../AES/$(*B).c
	$(COMPL_O2_W3)
$(7ZAES_OPT_OBJS): $(*B).cpp
	$(COMPL_O2)
$(CRYPTO_HASH_OBJS): ../../Crypto/Hash/$(*B).cpp
	$(COMPL_O2)
$(COMMON_OBJS): ../../../Common/$(*B).cpp
	$(COMPL)
$(WIN_OBJS): ../../../Windows/$(*B).cpp
	$(COMPL)
$(7ZIP_COMMON_OBJS): ../../Common/$(*B).cpp
	$(COMPL)
$(C_OBJS): ../../../../C/$(*B).c
	$(COMPL_O2)
