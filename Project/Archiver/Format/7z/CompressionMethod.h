// CompressionMethod.h

#pragma once

#ifndef __7Z_COMPRESSIONMETHOD_H
#define __7Z_COMPRESSIONMETHOD_H

#include "Windows/PropVariant.h"

#include "MethodInfo.h"

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
  bool MatchFinderIsDefined;

  #ifdef EXCLUDE_COM
  CSysString MatchFinderName;
  #else
  CLSID EncoderClassID;
  CLSID MatchFinderClassID;
  #endif

  CObjectVector<CProperty> CoderProperties;
  CObjectVector<CProperty> EncoderProperties;
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
  UINT32 MultiThreadMult;
  
  bool PasswordIsDefined;
  UString Password;

  bool IsEmpty() const { return (Methods.IsEmpty() && !PasswordIsDefined); }

  CCompressionMethodMode(): PasswordIsDefined(false), MultiThread(false) {}
};

}}

#endif
