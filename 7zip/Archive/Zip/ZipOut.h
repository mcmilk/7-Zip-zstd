// ZipOut.h

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
  UInt16 NumEntriesInCentaralDirectory;
  UInt32 CentralDirectorySize;
  UInt32 CentralDirectoryStartOffset;
  UInt16 CommentSize;
};

class COutArchive
{
  CMyComPtr<IOutStream> m_Stream;

  UInt32 m_BasePosition;
  UInt32 m_LocalFileHeaderSize;

  void WriteBytes(const void *buffer, UInt32 size);
  void WriteByte(Byte b);
  void WriteUInt16(UInt16 value);
  void WriteUInt32(UInt32 value);

public:
  void Create(IOutStream *outStream);
  void MoveBasePosition(UInt32 distanceToMove);
  UInt32 GetCurrentPosition() const { return m_BasePosition; };
  void PrepareWriteCompressedData(UInt16 fileNameLength);
  void WriteLocalHeader(const CItem &item);

  void WriteCentralHeader(const CItem &item);
  void WriteEndOfCentralDir(const COutArchiveInfo &archiveInfo);

  void CreateStreamForCompressing(IOutStream **outStream);
  void CreateStreamForCopying(ISequentialOutStream **outStream);
  void SeekToPackedDataPosition();
};

}}

#endif
