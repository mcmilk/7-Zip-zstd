// Archive/Zip/OutEngine.h

#pragma once

#ifndef __ZIP_OUTENGINE_H
#define __ZIP_OUTENGINE_H

#include "Archive/Zip/Header.h"
#include "Interface/IInOutStreams.h"
#include "Archive/Zip/ItemInfo.h"

namespace NArchive {
namespace NZip {

class COutArchiveInfo
{
public:
  UINT16 NumEntriesInCentaralDirectory;
  UINT32 CentralDirectorySize;
  UINT32 CentralDirectoryStartOffset;
  UINT16 CommentSize;
};

class COutArchive
{
  CComPtr<IOutStream> m_Stream;

  UINT32 m_BasePosition;
  UINT32 m_LocalFileHeaderSize;

  void WriteBytes(const void *aBuffer, UINT32 aSize);

public:
  void Create(IOutStream *aStream);
  void MoveBasePosition(UINT32 aDistanceToMove);
  UINT32 GetCurrentPosition() const { return m_BasePosition; };
  void PrepareWriteCompressedData(UINT16 aFileNameLength);
  void WriteLocalHeader(const CItemInfo &anItemInfo);

  void WriteCentralHeader(const CItemInfo &anItemInfo);
  void WriteEndOfCentralDir(const COutArchiveInfo &anArchiveInfo);

  void CreateStreamForCompressing(IOutStream **anOutStream);
  void CreateStreamForCopying(ISequentialOutStream **anOutStream);
};

}}

#endif
