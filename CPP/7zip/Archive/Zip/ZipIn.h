// Archive/ZipIn.h

#ifndef __ZIP_IN_H
#define __ZIP_IN_H

#include "../../../Common/MyCom.h"

#include "../../IStream.h"

#include "../../Common/InBuffer.h"

#include "ZipHeader.h"
#include "ZipItem.h"

API_FUNC_IsArc IsArc_Zip(const Byte *p, size_t size);

namespace NArchive {
namespace NZip {
  
class CItemEx: public CItem
{
public:
  UInt32 LocalFullHeaderSize; // including Name and Extra
  
  UInt64 GetLocalFullSize() const
    { return LocalFullHeaderSize + PackSize + (HasDescriptor() ? kDataDescriptorSize : 0); }
  UInt64 GetDataPosition() const
    { return LocalHeaderPos + LocalFullHeaderSize; }
};

struct CInArchiveInfo
{
  Int64 Base; /* Base offset of start of archive in stream.
                 Offsets in headers must be calculated from that Base.
                 Base is equal to MarkerPos for normal ZIPs.
                 Base can point to PE stub for some ZIP SFXs.
                 if CentralDir was read,
                   Base can be negative, if start of data is not available,
                 if CentralDirs was not read,
                   Base = ArcInfo.MarkerPos; */

  /* The following *Pos variables contain absolute offsets in Stream */
  UInt64 MarkerPos;  /* Pos of first signature, it can point to PK00 signature
                        = MarkerPos2      in most archives
                        = MarkerPos2 - 4  if there is PK00 signature */
  UInt64 MarkerPos2; // Pos of first local item signature in stream
  UInt64 FinishPos;  // Finish pos of archive data
  UInt64 FileEndPos; // Finish pos of stream

  UInt64 FirstItemRelatOffset; /* Relative offset of first local (read from cd) (relative to Base).
                                  = 0 in most archives
                                  = size of stub for some SFXs */
  bool CdWasRead;

  CByteBuffer Comment;

  CInArchiveInfo(): Base(0), MarkerPos(0), MarkerPos2(0), FinishPos(0), FileEndPos(0),
      FirstItemRelatOffset(0), CdWasRead(false) {}
  
  UInt64 GetPhySize() const { return FinishPos - Base; }
  UInt64 GetEmbeddedStubSize() const
  {
    if (CdWasRead)
      return FirstItemRelatOffset;
    return MarkerPos2 - Base;
  }
  bool ThereIsTail() const { return FileEndPos > FinishPos; }

  void Clear()
  {
    Base = 0;
    MarkerPos = 0;
    MarkerPos2 = 0;
    FinishPos = 0;
    FileEndPos = 0;

    FirstItemRelatOffset = 0;
    CdWasRead = false;

    Comment.Free();
  }
};

struct CProgressVirt
{
  virtual HRESULT SetCompletedLocal(UInt64 numFiles, UInt64 numBytes) = 0;
  virtual HRESULT SetTotalCD(UInt64 numFiles) = 0;
  virtual HRESULT SetCompletedCD(UInt64 numFiles) = 0;
};

struct CCdInfo
{
  UInt64 NumEntries;
  UInt64 Size;
  UInt64 Offset;

  void ParseEcd(const Byte *p);
  void ParseEcd64(const Byte *p);
};

class CInArchive
{
  CInBuffer _inBuffer;
  bool _inBufMode;
  UInt32 m_Signature;
  UInt64 m_Position;
  
  HRESULT Seek(UInt64 offset);
  HRESULT FindAndReadMarker(IInStream *stream, const UInt64 *searchHeaderSizeLimit);
  HRESULT IncreaseRealPosition(Int64 addValue);

  HRESULT ReadBytes(void *data, UInt32 size, UInt32 *processedSize);
  void SafeReadBytes(void *data, unsigned size);
  void ReadBuffer(CByteBuffer &buffer, unsigned size);
  Byte ReadByte();
  UInt16 ReadUInt16();
  UInt32 ReadUInt32();
  UInt64 ReadUInt64();
  void Skip(unsigned num);
  void Skip64(UInt64 num);
  void ReadFileName(unsigned nameSize, AString &dest);

  bool ReadExtra(unsigned extraSize, CExtraBlock &extraBlock,
      UInt64 &unpackSize, UInt64 &packSize, UInt64 &localHeaderOffset, UInt32 &diskStartNumber);
  bool ReadLocalItem(CItemEx &item);
  HRESULT ReadLocalItemDescriptor(CItemEx &item);
  HRESULT ReadCdItem(CItemEx &item);
  HRESULT TryEcd64(UInt64 offset, CCdInfo &cdInfo);
  HRESULT FindCd(CCdInfo &cdInfo);
  HRESULT TryReadCd(CObjectVector<CItemEx> &items, UInt64 cdOffset, UInt64 cdSize, CProgressVirt *progress);
  HRESULT ReadCd(CObjectVector<CItemEx> &items, UInt64 &cdOffset, UInt64 &cdSize, CProgressVirt *progress);
  HRESULT ReadLocals(CObjectVector<CItemEx> &localItems, CProgressVirt *progress);

  HRESULT ReadHeaders2(CObjectVector<CItemEx> &items, CProgressVirt *progress);
public:
  CInArchiveInfo ArcInfo;
  
  bool IsArc;
  bool IsZip64;
  bool HeadersError;
  bool HeadersWarning;
  bool ExtraMinorError;
  bool UnexpectedEnd;
  bool NoCentralDir;
  
  CMyComPtr<IInStream> Stream;
  
  void Close();
  HRESULT Open(IInStream *stream, const UInt64 *searchHeaderSizeLimit);
  HRESULT ReadHeaders(CObjectVector<CItemEx> &items, CProgressVirt *progress);

  bool IsOpen() const { return Stream != NULL; }
  bool AreThereErrors() const { return HeadersError || UnexpectedEnd; }

  bool IsLocalOffsetOK(const CItemEx &item) const
  {
    if (item.FromLocal)
      return true;
    return /* ArcInfo.Base >= 0 || */ ArcInfo.Base + (Int64)item.LocalHeaderPos >= 0;
  }

  HRESULT ReadLocalItemAfterCdItem(CItemEx &item);
  HRESULT ReadLocalItemAfterCdItemFull(CItemEx &item);

  ISequentialInStream *CreateLimitedStream(UInt64 position, UInt64 size);

  UInt64 GetOffsetInStream(UInt64 offsetFromArc) const { return ArcInfo.Base + offsetFromArc; }

  bool CanUpdate() const
  {
    if (AreThereErrors())
      return false;
    if (ArcInfo.Base < 0)
      return false;
    if ((Int64)ArcInfo.MarkerPos2 < ArcInfo.Base)
      return false;
   
    // 7-zip probably can update archives with embedded stubs.
    // we just disable that feature for more safety.
    if (ArcInfo.GetEmbeddedStubSize() != 0)
      return false;

    if (ArcInfo.ThereIsTail())
      return false;
    return true;
  }
};
  
}}
  
#endif
