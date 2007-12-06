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
  enum ECauseType
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
  CInArchiveException(ECauseType cause): Cause(cause) {}
};

class CInArchiveInfo
{
public:
  UInt64 Base;
  UInt64 StartPosition;
  CByteBuffer Comment;
  CInArchiveInfo(): Base(0), StartPosition(0) {}
  void Clear() 
  { 
    Base = 0;
    StartPosition = 0;
    Comment.SetCapacity(0); 
  }
};

class CProgressVirt
{
public:
  STDMETHOD(SetCompleted)(const UInt64 *numFiles) PURE;
};

struct CCdInfo
{
  // UInt64 NumEntries;
  UInt64 Size;
  UInt64 Offset;
};

class CInArchive
{
  CMyComPtr<IInStream> m_Stream;
  UInt32 m_Signature;
  UInt64 m_StreamStartPosition;
  UInt64 m_Position;
  AString m_NameBuffer;
  
  HRESULT Seek(UInt64 offset);

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
  
  void IncreaseRealPosition(UInt64 addValue);
 
  void ReadExtra(UInt32 extraSize, CExtraBlock &extraBlock, 
      UInt64 &unpackSize, UInt64 &packSize, UInt64 &localHeaderOffset, UInt32 &diskStartNumber);
  HRESULT ReadLocalItem(CItemEx &item);
  HRESULT ReadLocalItemDescriptor(CItemEx &item);
  HRESULT ReadCdItem(CItemEx &item);
  HRESULT TryEcd64(UInt64 offset, CCdInfo &cdInfo);
  HRESULT FindCd(CCdInfo &cdInfo);
  HRESULT TryReadCd(CObjectVector<CItemEx> &items, UInt64 cdOffset, UInt64 cdSize);
  HRESULT ReadCd(CObjectVector<CItemEx> &items, UInt64 &cdOffset, UInt64 &cdSize);
  HRESULT ReadLocalsAndCd(CObjectVector<CItemEx> &items, CProgressVirt *progress, UInt64 &cdOffset);
public:
  CInArchiveInfo m_ArchiveInfo;
  bool IsZip64;

  HRESULT ReadHeaders(CObjectVector<CItemEx> &items, CProgressVirt *progress);
  HRESULT ReadLocalItemAfterCdItem(CItemEx &item);
  HRESULT ReadLocalItemAfterCdItemFull(CItemEx &item);
  bool Open(IInStream *inStream, const UInt64 *searchHeaderSizeLimit);
  void Close();
  void GetArchiveInfo(CInArchiveInfo &archiveInfo) const;
  bool SeekInArchive(UInt64 position);
  ISequentialInStream *CreateLimitedStream(UInt64 position, UInt64 size);
  IInStream* CreateStream();
};
  
}}
  
#endif
