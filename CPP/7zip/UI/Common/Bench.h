// Bench.h

#ifndef __7ZIP_BENCH_H
#define __7ZIP_BENCH_H

#include "../../Common/CreateCoder.h"

struct CBenchInfo
{
  UInt64 GlobalTime;
  UInt64 GlobalFreq;
  UInt64 UserTime;
  UInt64 UserFreq;
  UInt64 UnpackSize;
  UInt64 PackSize;
  UInt32 NumIterations;
  CBenchInfo(): NumIterations(0) {}
};

struct IBenchCallback
{
  virtual HRESULT SetEncodeResult(const CBenchInfo &info, bool final) = 0;
  virtual HRESULT SetDecodeResult(const CBenchInfo &info, bool final) = 0;
};

UInt64 GetUsage(const CBenchInfo &benchOnfo);
UInt64 GetRatingPerUsage(const CBenchInfo &info, UInt64 rating);
UInt64 GetCompressRating(UInt32 dictionarySize, UInt64 elapsedTime, UInt64 freq, UInt64 size);
UInt64 GetDecompressRating(UInt64 elapsedTime, UInt64 freq, UInt64 outSize, UInt64 inSize, UInt32 numIterations);

HRESULT LzmaBench(
  DECL_EXTERNAL_CODECS_LOC_VARS
  UInt32 numThreads, UInt32 dictionarySize, IBenchCallback *callback);

const int kBenchMinDicLogSize = 18;

UInt64 GetBenchMemoryUsage(UInt32 numThreads, UInt32 dictionary);

bool CrcInternalTest();
HRESULT CrcBench(UInt32 numThreads, UInt32 bufferSize, UInt64 &speed);

#endif
