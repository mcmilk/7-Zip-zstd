// 7z/Header.h

#pragma once

#ifndef __7Z_HEADER_H
#define __7Z_HEADER_H

#include "Common/Types.h"
#include "Common/CRC.h"

#include "MethodInfo.h"

namespace NArchive {
namespace N7z {

#pragma pack( push, Pragma7zHeaders)
#pragma pack( push, 1)

const kSignatureSize = 6;
extern BYTE kSignature[kSignatureSize];

struct CArchiveVersion
{
  BYTE Major;
  BYTE Minor;
};

struct CStartHeader
{
  UINT64 NextHeaderOffset;
  UINT64 NextHeaderSize;
  UINT32 NextHeaderCRC;
};

namespace NID
{
  enum EEnum
  {
    kEnd,

    kHeader,

    kArchiveProperties,
    
    kAdditionalStreamsInfo,
    kMainStreamsInfo,
    kFilesInfo,
    
    kPackInfo,
    kUnPackInfo,
    kSubStreamsInfo,

    kSize,
    kCRC,

    kFolder,

    kCodersUnPackSize,
    kNumUnPackStream,

    kEmptyStream,
    kEmptyFile,
    kAnti,

    kName,
    kCreationTime,
    kLastAccessTime,
    kLastWriteTime,
    kWinAttributes,
    kComment,
  };
}


#pragma pack(pop)
#pragma pack(pop, Pragma7zHeaders)

const BYTE kMajorVersion = 0;

}}


#endif
