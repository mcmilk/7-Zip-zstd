// 7zUpdate.h

#pragma once

#ifndef __7Z_UPDATE_H
#define __7Z_UPDATE_H

#include "7zIn.h"
#include "7zCompressionMode.h"

#include "../IArchive.h"

namespace NArchive {
namespace N7z {

struct CUpdateRange
{
  UINT64 Position; 
  UINT64 Size;
  CUpdateRange() {};
  CUpdateRange(UINT64 position, UINT64 size): Position(position), Size(size) {};
};

struct CUpdateItem
{
  bool NewData;
  bool NewProperties;
  int IndexInArchive;
  int IndexInClient;
  
  UINT32 Attributes;
  FILETIME CreationTime;
  FILETIME LastWriteTime;

  UINT64 Size;
  UString Name;
  
  bool IsAnti;
  bool IsDirectory;

  bool CreationTimeIsDefined;
  bool LastWriteTimeIsDefined;
  bool AttributesAreDefined;

  const bool HasStream() const 
    { return !IsDirectory && !IsAnti && Size != 0; }
  CUpdateItem():  IsAnti(false) {}
  void SetDirectoryStatusFromAttributes()
    { IsDirectory = ((Attributes & FILE_ATTRIBUTE_DIRECTORY) != 0); };

  int GetExtensionPos() const;
  UString GetExtension() const;
};

HRESULT Update(const NArchive::N7z::CArchiveDatabaseEx &database,
    CObjectVector<CUpdateItem> &updateItems,
    IOutStream *outStream,
    IInStream *inStream,
    CInArchiveInfo *inArchiveInfo,
    const CCompressionMethodMode &method,
    const CCompressionMethodMode *headerMethod,
    bool useFilters,
    bool maxFilter,
    bool useAdditionalHeaderStreams, 
    bool compressMainHeader,
    IArchiveUpdateCallback *updateCallback,
    UINT64 numSolidFiles, UINT64 numSolidBytes, bool solidExtension,
    bool removeSfxBlock);

}}

#endif
