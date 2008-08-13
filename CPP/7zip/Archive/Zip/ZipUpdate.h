// Zip/Update.h

#ifndef __ZIP_UPDATE_H
#define __ZIP_UPDATE_H

#include "../../ICoder.h"
#include "../IArchive.h"

#include "../../Common/CreateCoder.h"

#include "ZipCompressionMode.h"
#include "ZipIn.h"

namespace NArchive {
namespace NZip {

struct CUpdateRange
{
  UInt64 Position;
  UInt64 Size;
  CUpdateRange() {};
  CUpdateRange(UInt64 position, UInt64 size): Position(position), Size(size) {};
};

struct CUpdateItem
{
  bool NewData;
  bool NewProperties;
  bool IsDir;
  bool NtfsTimeIsDefined;
  bool IsUtf8;
  int IndexInArchive;
  int IndexInClient;
  UInt32 Attributes;
  UInt32 Time;
  UInt64 Size;
  AString Name;
  // bool Commented;
  // CUpdateRange CommentRange;
  FILETIME NtfsMTime;
  FILETIME NtfsATime;
  FILETIME NtfsCTime;

  CUpdateItem(): NtfsTimeIsDefined(false), IsUtf8(false), Size(0) {}
};

HRESULT Update(
    DECL_EXTERNAL_CODECS_LOC_VARS
    const CObjectVector<CItemEx> &inputItems,
    const CObjectVector<CUpdateItem> &updateItems,
    ISequentialOutStream *seqOutStream,
    CInArchive *inArchive,
    CCompressionMethodMode *compressionMethodMode,
    IArchiveUpdateCallback *updateCallback);

}}

#endif
