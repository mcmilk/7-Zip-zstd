// 7z/Out.h

#pragma once

#ifndef __7Z_OUT_H
#define __7Z_OUT_H

#include "7zHeader.h"
#include "7zItem.h"
#include "7zCompressionMode.h"
#include "7zEncode.h"

#include "../../Common/OutBuffer.h"
#include "../../../Common/DynamicBuffer.h"
#include "../../../Common/CRC.h"

namespace NArchive {
namespace N7z {

class CWriteBufferLoc
{
  BYTE *_data;
  UINT32 _size;
  UINT32 _pos;
public:
  CWriteBufferLoc(): _size(0), _pos(0) {}
  void Init(BYTE *data, UINT32 size)  
  { 
    _pos = 0;
    _data = data;
    _size = size; 
  }
  HRESULT Write(const void *data, UINT32 size)
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
  UINT32 _pos;
public:
  CWriteDynamicBuffer(): _pos(0) {}
  void Init()  
  { 
    _pos = 0;
  }
  void Write(const void *data, UINT32 size)
  {
    if (_pos + size > _buffer.GetCapacity())
      _buffer.EnsureCapacity(_pos + size);
    memmove(((BYTE *)_buffer) +_pos, data, size);
    _pos += size;
  }
  operator BYTE *() { return (BYTE *)_buffer; };
  operator const BYTE *() const { return (const BYTE *)_buffer; };
  UINT32 GetSize() const { return _pos; }
};


class COutArchive
{
  UINT64 _prefixHeaderPos;

  HRESULT WriteBytes(const void *data, UINT32 size);
  HRESULT WriteBytes2(const void *data, UINT32 size);
  HRESULT WriteBytes2(const CByteBuffer &data);
  HRESULT WriteByte2(BYTE b);
  HRESULT WriteNumber(UINT64 value);
  HRESULT WriteID(UINT64 value)
  {
    return WriteNumber(value);
  }

  HRESULT WriteFolderHeader(const CFolder &itemInfo);
  HRESULT WriteFileHeader(const CFileItem &itemInfo);
  HRESULT WriteBoolVector(const CBoolVector &boolVector);
  HRESULT WriteHashDigests(
      const CRecordVector<bool> &digestsDefined,
      const CRecordVector<UINT32> &hashDigests);

  HRESULT WritePackInfo(
      UINT64 dataOffset,
      const CRecordVector<UINT64> &packSizes,
      const CRecordVector<bool> &packCRCsDefined,
      const CRecordVector<UINT32> &packCRCs);

  HRESULT WriteUnPackInfo(
      bool externalFolders,
      UINT64 externalFoldersStreamIndex,
      const CObjectVector<CFolder> &folders);

  HRESULT WriteSubStreamsInfo(
      const CObjectVector<CFolder> &folders,
      const CRecordVector<UINT64> &numUnPackStreamsInFolders,
      const CRecordVector<UINT64> &unPackSizes,
      const CRecordVector<bool> &digestsDefined,
      const CRecordVector<UINT32> &hashDigests);

  HRESULT WriteStreamsInfo(
      UINT64 dataOffset,
      const CRecordVector<UINT64> &packSizes,
      const CRecordVector<bool> &packCRCsDefined,
      const CRecordVector<UINT32> &packCRCs,
      bool externalFolders,
      UINT64 externalFoldersStreamIndex,
      const CObjectVector<CFolder> &folders,
      const CRecordVector<UINT64> &numUnPackStreamsInFolders,
      const CRecordVector<UINT64> &unPackSizes,
      const CRecordVector<bool> &digestsDefined,
      const CRecordVector<UINT32> &hashDigests);


  HRESULT WriteTime(const CObjectVector<CFileItem> &files, BYTE type,
      bool isExternal, int externalDataIndex);

  HRESULT EncodeStream(CEncoder &encoder, const BYTE *data, UINT32 dataSize,
      CRecordVector<UINT64> &packSizes, CObjectVector<CFolder> &folders);
  HRESULT EncodeStream(CEncoder &encoder, const CByteBuffer &data, 
      CRecordVector<UINT64> &packSizes, CObjectVector<CFolder> &folders);
  HRESULT WriteHeader(const CArchiveDatabase &database,
      const CCompressionMethodMode *options, 
      UINT64 &headerOffset);
  
  bool _mainMode;

  bool _dynamicMode;

  bool _countMode;
  UINT32 _countSize;
  COutBuffer _outByte;
  CWriteBufferLoc _outByte2;
  CWriteDynamicBuffer _dynamicBuffer;
  CCRC _crc;

public:
  CMyComPtr<IOutStream> Stream;
  HRESULT Create(IOutStream *stream);
  void Close();
  HRESULT SkeepPrefixArchiveHeader();
  HRESULT WriteDatabase(const CArchiveDatabase &database,
      const CCompressionMethodMode *options, 
      bool useAdditionalHeaderStreams, 
      bool compressMainHeader);
};

}}

#endif
