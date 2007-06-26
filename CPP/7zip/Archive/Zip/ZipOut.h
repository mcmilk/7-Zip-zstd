// ZipOut.h

#ifndef __ZIP_OUT_H
#define __ZIP_OUT_H

#include "Common/MyCom.h"

#include "../../IStream.h"
#include "../../Common/OutBuffer.h"

#include "ZipItem.h"

namespace NArchive {
namespace NZip {

// can throw CSystemException and COutBufferException
  
class COutArchive
{
  CMyComPtr<IOutStream> m_Stream;
  COutBuffer m_OutBuffer;

  UInt64 m_BasePosition;
  UInt32 m_LocalFileHeaderSize;
  UInt32 m_ExtraSize;
  bool m_IsZip64;

  void WriteBytes(const void *buffer, UInt32 size);
  void WriteByte(Byte b);
  void WriteUInt16(UInt16 value);
  void WriteUInt32(UInt32 value);
  void WriteUInt64(UInt64 value);

  void WriteExtraHeader(const CItem &item);
  void WriteCentralHeader(const CItem &item);
  void WriteExtra(const CExtraBlock &extra);
  void SeekTo(UInt64 offset);
public:
  void Create(IOutStream *outStream);
  void MoveBasePosition(UInt64 distanceToMove);
  UInt64 GetCurrentPosition() const { return m_BasePosition; };
  void PrepareWriteCompressedDataZip64(UInt16 fileNameLength, bool isZip64, bool aesEncryption);
  void PrepareWriteCompressedData(UInt16 fileNameLength, UInt64 unPackSize, bool aesEncryption);
  void PrepareWriteCompressedData2(UInt16 fileNameLength, UInt64 unPackSize, UInt64 packSize, bool aesEncryption);
  void WriteLocalHeader(const CLocalItem &item);

  void WriteCentralDir(const CObjectVector<CItem> &items, const CByteBuffer &comment);

  void CreateStreamForCompressing(IOutStream **outStream);
  void CreateStreamForCopying(ISequentialOutStream **outStream);
  void SeekToPackedDataPosition();
};

}}

#endif
