// ZipOut.h

#pragma once

#ifndef __ZIP_OUT_H
#define __ZIP_OUT_H

#include "Common/MyCom.h"

#include "../../IStream.h"

#include "ZipHeader.h"
#include "ZipItem.h"

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
  CMyComPtr<IOutStream> m_Stream;

  UINT32 m_BasePosition;
  UINT32 m_LocalFileHeaderSize;

  void WriteBytes(const void *buffer, UINT32 size);

public:
  void Create(IOutStream *outStream);
  void MoveBasePosition(UINT32 distanceToMove);
  UINT32 GetCurrentPosition() const { return m_BasePosition; };
  void PrepareWriteCompressedData(UINT16 fileNameLength);
  void WriteLocalHeader(const CItem &item);

  void WriteCentralHeader(const CItem &item);
  void WriteEndOfCentralDir(const COutArchiveInfo &archiveInfo);

  void CreateStreamForCompressing(IOutStream **outStream);
  void CreateStreamForCopying(ISequentialOutStream **outStream);
  void SeekToPackedDataPosition();
};

}}

#endif
