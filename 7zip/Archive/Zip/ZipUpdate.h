// Zip/Update.h

#pragma once

#ifndef __ZIP_UPDATE_H
#define __ZIP_UPDATE_H

#include "Common/Vector.h"
#include "Common/Types.h"

#include "../../ICoder.h"
#include "../IArchive.h"

#include "ZipCompressionMode.h"
#include "ZipIn.h"

namespace NArchive {
namespace NZip {

struct CUpdateRange
{
  UINT32 Position; 
  UINT32 Size;
  CUpdateRange() {};
  CUpdateRange(UINT32 position, UINT32 size):
      Position(position), Size(size) {};
};

struct CUpdateItem
{
  bool NewData;
  bool NewProperties;
  bool IsDirectory;
  int IndexInArchive;
  int IndexInClient;
  UINT32 Attributes;
  UINT32 Time;
  UINT32 Size;
  AString Name;
  // bool ExistInArchive;
  bool Commented;
  CUpdateRange CommentRange;
  /*
  bool IsDirectory() const 
    { return ((Attributes & FILE_ATTRIBUTE_DIRECTORY) != 0); };
  */
};

HRESULT Update(
    const CObjectVector<CItemEx> &inputItems,
    const CObjectVector<CUpdateItem> &updateItems,
    IOutStream *outStream,
    CInArchive *inArchive,
    CCompressionMethodMode *compressionMethodMode,
    IArchiveUpdateCallback *updateCallback);

}}

#endif
