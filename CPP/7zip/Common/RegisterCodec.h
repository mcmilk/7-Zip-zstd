// RegisterCodec.h

#ifndef __REGISTER_CODEC_H
#define __REGISTER_CODEC_H

#include "../Common/MethodId.h"
#include "../ICoder.h"

typedef void * (*CreateCodecP)();
struct CCodecInfo
{
  CreateCodecP CreateDecoder;
  CreateCodecP CreateEncoder;
  CMethodId Id;
  const wchar_t *Name;
  UInt32 NumInStreams;
  bool IsFilter;
};

void RegisterCodec(const CCodecInfo *codecInfo) throw();

#define REGISTER_CODEC_NAME(x) CRegisterCodec ## x

#define REGISTER_CODEC(x) struct REGISTER_CODEC_NAME(x) { \
    REGISTER_CODEC_NAME(x)() { RegisterCodec(&g_CodecInfo); }}; \
    static REGISTER_CODEC_NAME(x) g_RegisterCodec;

#define REGISTER_CODECS_NAME(x) CRegisterCodecs ## x
#define REGISTER_CODECS(x) struct REGISTER_CODECS_NAME(x) { \
    REGISTER_CODECS_NAME(x)() { for (unsigned i = 0; i < ARRAY_SIZE(g_CodecsInfo); i++) \
    RegisterCodec(&g_CodecsInfo[i]); }}; \
    static REGISTER_CODECS_NAME(x) g_RegisterCodecs;


struct CHasherInfo
{
  IHasher * (*CreateHasher)();
  CMethodId Id;
  const wchar_t *Name;
  UInt32 DigestSize;
};

void RegisterHasher(const CHasherInfo *hasher) throw();

#define REGISTER_HASHER_NAME(x) CRegisterHasher ## x

#define REGISTER_HASHER(x) struct REGISTER_HASHER_NAME(x) { \
    REGISTER_HASHER_NAME(x)() { RegisterHasher(&g_HasherInfo); }}; \
    static REGISTER_HASHER_NAME(x) g_RegisterHasher;

#endif
