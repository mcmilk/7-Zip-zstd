// Archive/ZipIn.h

#ifndef __ZIP_IN_H
#define __ZIP_IN_H

#include "Common/MyCom.h"
#include "../../IStream.h"

#include "ZipHeader.h"
#include "ZipItemEx.h"

namespace NArchive {
namespace NZip {
  
class CInArchiveException
{
public:
  enum CCauseType
  {
    kUnexpectedEndOfArchive = 0,
    kArchiceHeaderCRCError,
    kFileHeaderCRCError,
    kIncorrectArchive,
    kDataDescroptorsAreNotSupported,
    kMultiVolumeArchiveAreNotSupported,
    kReadStreamError,
    kSeekStreamError
  } 
  Cause;
  CInArchiveException(CCauseType cause): Cause(cause) {}
};

class CInArchiveInfo
{
public:
  UInt64 StartPosition;
  CByteBuffer Comment;
  void Clear() { Comment.SetCapacity(0); }
};

class CProgressVirt
{
public:
  STDMETHOD(SetCompleted)(const UInt64 *numFiles) PURE;
};

class CInArchive
{
  CMyComPtr<IInStream> m_Stream;
  UInt32 m_Signature;
  UInt64 m_StreamStartPosition;
  UInt64 m_Position;
  CInArchiveInfo m_ArchiveInfo;
  AString m_NameBuffer;
  
  bool FindAndReadMarker(const UInt64 *searchHeaderSizeLimit);
  bool ReadUInt32(UInt32 &signature);
  AString ReadFileName(UInt32 nameSize);
  
  HRESULT ReadBytes(void *data, UInt32 size, UInt32 *processedSize);
  bool ReadBytesAndTestSize(void *data, UInt32 size);
  void SafeReadBytes(void *data, UInt32 size);
  void ReadBuffer(CByteBuffer &buffer, UInt32 size);
  Byte ReadByte();
  UInt16 ReadUInt16();
  UInt32 ReadUInt32();
  UInt64 ReadUInt64();
  
  void IncreasePositionValue(UInt64 addValue);
  void IncreaseRealPosition(UInt64 addValue);
  void ThrowIncorrectArchiveException();
 
  void ReadExtra(UInt32 extraSize, CExtraBlock &extraBlock, 
      UInt64 &unpackSize, UInt64 &packSize, UInt64 &localHeaderOffset, UInt32 &diskStartNumber);
public:
  HRESULT ReadHeaders(CObjectVector<CItemEx> &items, CProgressVirt *progress);
  bool Open(IInStream *inStream, const UInt64 *searchHeaderSizeLimit);
  void Close();
  void GetArchiveInfo(CInArchiveInfo &archiveInfo) const;
  void DirectGetBytes(void *data, UInt32 num);
  bool SeekInArchive(UInt64 position);
  ISequentialInStream *CreateLimitedStream(UInt64 position, UInt64 size);
  IInStream* CreateStream();
};
  
}}
  
#endif
