// InStreamWithCRC.h

#pragma once

#ifndef __INSTREAMWITHCRC_H
#define __INSTREAMWITHCRC_H

#include "Common/CRC.h"
#include "Interface/IInOutStreams.h"

//////////////////////////////////////
// CInStreamWithCRC

class CInStreamWithCRC: 
  public IInStream,
  public CComObjectRoot
{
public:
BEGIN_COM_MAP(CInStreamWithCRC)
  COM_INTERFACE_ENTRY(IInStream)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CInStreamWithCRC)

DECLARE_NO_REGISTRY()

  STDMETHOD(Read)(void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(ReadPart)(void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(Seek)(INT64 offset, UINT32 seekOrigin, UINT64 *newPosition);
private:
  CCRC _crc;
  CComPtr<IInStream> _stream;
public:
  void Init(IInStream *stream)
  {
    _stream = stream;
    _crc.Init();
  }
  void ReleaseStream()
    { _stream.Release(); }
  UINT32 GetCRC() const { return _crc.GetDigest(); }
};

#endif