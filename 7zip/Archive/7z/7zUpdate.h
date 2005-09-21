// 7zUpdate.h

#ifndef __7Z_UPDATE_H
#define __7Z_UPDATE_H

#include "7zIn.h"
#include "7zCompressionMode.h"

#include "../IArchive.h"

namespace NArchive {
namespace N7z {

struct CUpdateItem
{
  bool NewData;
  bool NewProperties;
  int IndexInArchive;
  int IndexInClient;
  
  UInt32 Attributes;
  FILETIME CreationTime;
  FILETIME LastWriteTime;

  UInt64 Size;
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

struct CUpdateOptions
{
  const CCompressionMethodMode *Method;
  const CCompressionMethodMode *HeaderMethod;
  bool UseFilters;
  bool MaxFilter;
  bool UseAdditionalHeaderStreams;
  bool CompressMainHeader;
  UInt64 NumSolidFiles;
  UInt64 NumSolidBytes;
  bool SolidExtension;
  bool RemoveSfxBlock;
  bool VolumeMode;
};

HRESULT Update(
    IInStream *inStream,
    const CArchiveDatabaseEx *database,
    const CObjectVector<CUpdateItem> &updateItems,
    ISequentialOutStream *seqOutStream,
    IArchiveUpdateCallback *updateCallback,
    const CUpdateOptions &options);
}}

#endif
