// InStreamWithCRC.h

#pragma once

#ifndef __INSTREAMWITHCRC_H
#define __INSTREAMWITHCRC_H

#include "../../../Common/CRC.h"
#include "../../../Common/MyCom.h"
#include "../../IStream.h"

class CInStreamWithCRC: 
  public IInStream,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP

  STDMETHOD(Read)(void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(ReadPart)(void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(Seek)(INT64 offset, UINT32 seekOrigin, UINT64 *newPosition);
private:
  CMyComPtr<IInStream> _stream;
  UINT64 _size;
  CCRC _crc;
public:
  void Init(IInStream *stream)
  {
    _stream = stream;
    _size = 0;
    _crc.Init();
  }
  void ReleaseStream() { _stream.Release(); }
  UINT32 GetCRC() const { return _crc.GetDigest(); }
  UINT64 GetSize() const { return _size; }
};

#endif