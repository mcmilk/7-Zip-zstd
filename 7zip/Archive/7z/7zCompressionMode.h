// 7zCompressionMode.h

#pragma once

#ifndef __7Z_COMPRESSION_MODE_H
#define __7Z_COMPRESSION_MODE_H

#include "../../../Windows/PropVariant.h"

#include "7zMethodID.h"

namespace NArchive {
namespace N7z {

struct CProperty
{
  PROPID PropID;
  NWindows::NCOM::CPropVariant Value;
};

struct CMethodInfoEx
{
  CMethodID MethodID;
  UINT32 NumInStreams;
  UINT32 NumOutStreams;
  bool IsSimpleCoder() const 
    { return (NumInStreams == 1) && (NumOutStreams == 1); }
};

struct CMethodFull
{
  CMethodInfoEx MethodInfoEx;

  #ifdef EXCLUDE_COM
  #else
  CLSID EncoderClassID;
  CSysString FilePath;
  #endif

  CObjectVector<CProperty> CoderProperties;
};

struct CBind
{
  UINT32 InCoder;
  UINT32 InStream;
  UINT32 OutCoder;
  UINT32 OutStream;
};

struct CCompressionMethodMode
{
  CObjectVector<CMethodFull> Methods;
  CRecordVector<CBind> Binds;
  bool MultiThread;
  // UINT32 MultiThreadMult;
  
  bool PasswordIsDefined;
  UString Password;

  bool IsEmpty() const { return (Methods.IsEmpty() && !PasswordIsDefined); }

  CCompressionMethodMode(): PasswordIsDefined(false), MultiThread(false) {}
};

}}

#endif
