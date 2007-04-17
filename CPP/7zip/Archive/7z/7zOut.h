// 7z/Out.h

#ifndef __7Z_OUT_H
#define __7Z_OUT_H

#include "7zHeader.h"
#include "7zItem.h"
#include "7zCompressionMode.h"
#include "7zEncode.h"

#include "../../Common/OutBuffer.h"
#include "../../../Common/DynamicBuffer.h"

namespace NArchive {
namespace N7z {

class CWriteBufferLoc
{
  Byte *_data;
  size_t _size;
  size_t _pos;
public:
  CWriteBufferLoc(): _size(0), _pos(0) {}
  void Init(Byte *data, size_t size)  
  { 
    _pos = 0;
    _data = data;
    _size = size; 
  }
  HRESULT Write(const void *data, size_t size)
  {
    if (_pos + size > _size)
      return E_FAIL;
    memmove(_data + _pos, data, size);
    _pos += size;
    return S_OK; 
  }
};

class CWriteDynamicBuffer
{
  CByteDynamicBuffer _buffer;
  size_t _pos;
public:
  CWriteDynamicBuffer(): _pos(0) {}
  void Init()  
  { 
    _pos = 0;
  }
  void Write(const void *data, size_t size)
  {
    if (_pos + size > _buffer.GetCapacity())
      _buffer.EnsureCapacity(_pos + size);
    memmove(((Byte *)_buffer) +_pos, data, size);
    _pos += size;
  }
  operator Byte *() { return (Byte *)_buffer; };
  operator const Byte *() const { return (const Byte *)_buffer; };
  size_t GetSize() const { return _pos; }
};

struct CHeaderOptions
{
  // bool UseAdditionalHeaderStreams;
  bool CompressMainHeader;
  bool WriteModified;
  bool WriteCreated;
  bool WriteAccessed;

  CHeaderOptions(): 
      // UseAdditionalHeaderStreams(false), 
      CompressMainHeader(true),
      WriteModified(true),
      WriteCreated(false),
      WriteAccessed(false) {} 
};

class COutArchive
{
  UInt64 _prefixHeaderPos;

  HRESULT WriteDirect(const void *data, UInt32 size);
  HRESULT WriteDirectByte(Byte b) { return WriteDirect(&b, 1); }
  HRESULT WriteDirectUInt32(UInt32 value);
  HRESULT WriteDirectUInt64(UInt64 value);
  
  HRESULT WriteBytes(const void *data, size_t size);
  HRESULT WriteBytes(const CByteBuffer &data);
  HRESULT WriteByte(Byte b);
  HRESULT WriteUInt32(UInt32 value);
  HRESULT WriteNumber(UInt64 value);
  HRESULT WriteID(UInt64 value) { return WriteNumber(value); }

  HRESULT WriteFolder(const CFolder &folder);
  HRESULT WriteFileHeader(const CFileItem &itemInfo);
  HRESULT WriteBoolVector(const CBoolVector &boolVector);
  HRESULT WriteHashDigests(
      const CRecordVector<bool> &digestsDefined,
      const CRecordVector<UInt32> &hashDigests);

  HRESULT WritePackInfo(
      UInt64 dataOffset,
      const CRecordVector<UInt64> &packSizes,
      const CRecordVector<bool> &packCRCsDefined,
      const CRecordVector<UInt32> &packCRCs);

  HRESULT WriteUnPackInfo(const CObjectVector<CFolder> &folders);

  HRESULT WriteSubStreamsInfo(
      const CObjectVector<CFolder> &folders,
      const CRecordVector<CNum> &numUnPackStreamsInFolders,
      const CRecordVector<UInt64> &unPackSizes,
      const CRecordVector<bool> &digestsDefined,
      const CRecordVector<UInt32> &hashDigests);

  /*
  HRESULT WriteStreamsInfo(
      UInt64 dataOffset,
      const CRecordVector<UInt64> &packSizes,
      const CRecordVector<bool> &packCRCsDefined,
      const CRecordVector<UInt32> &packCRCs,
      bool externalFolders,
      UInt64 externalFoldersStreamIndex,
      const CObjectVector<CFolder> &folders,
      const CRecordVector<CNum> &numUnPackStreamsInFolders,
      const CRecordVector<UInt64> &unPackSizes,
      const CRecordVector<bool> &digestsDefined,
      const CRecordVector<UInt32> &hashDigests);
  */


  HRESULT WriteTime(const CObjectVector<CFileItem> &files, Byte type);

  HRESULT EncodeStream(
      DECL_EXTERNAL_CODECS_LOC_VARS
      CEncoder &encoder, const Byte *data, size_t dataSize,
      CRecordVector<UInt64> &packSizes, CObjectVector<CFolder> &folders);
  HRESULT EncodeStream(
      DECL_EXTERNAL_CODECS_LOC_VARS
      CEncoder &encoder, const CByteBuffer &data, 
      CRecordVector<UInt64> &packSizes, CObjectVector<CFolder> &folders);
  HRESULT WriteHeader(
      const CArchiveDatabase &database,
      const CHeaderOptions &headerOptions,
      UInt64 &headerOffset);
  
  bool _mainMode;

  bool _dynamicMode;

  bool _countMode;
  size_t _countSize;
  COutBuffer _outByte;
  CWriteBufferLoc _outByte2;
  CWriteDynamicBuffer _dynamicBuffer;
  UInt32 _crc;

  #ifdef _7Z_VOL
  bool _endMarker;
  #endif

  HRESULT WriteSignature();
  #ifdef _7Z_VOL
  HRESULT WriteFinishSignature();
  #endif
  HRESULT WriteStartHeader(const CStartHeader &h);
  #ifdef _7Z_VOL
  HRESULT WriteFinishHeader(const CFinishHeader &h);
  #endif
  CMyComPtr<IOutStream> Stream;
public:

  COutArchive() { _outByte.Create(1 << 16); }
  CMyComPtr<ISequentialOutStream> SeqStream;
  HRESULT Create(ISequentialOutStream *stream, bool endMarker);
  void Close();
  HRESULT SkeepPrefixArchiveHeader();
  HRESULT WriteDatabase(
      DECL_EXTERNAL_CODECS_LOC_VARS
      const CArchiveDatabase &database,
      const CCompressionMethodMode *options, 
      const CHeaderOptions &headerOptions);

  #ifdef _7Z_VOL
  static UInt32 GetVolHeadersSize(UInt64 dataSize, int nameLength = 0, bool props = false);
  static UInt64 GetVolPureSize(UInt64 volSize, int nameLength = 0, bool props = false);
  #endif

};

}}

#endif
