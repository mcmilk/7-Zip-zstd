// OutStreamWithCRC.h

#pragma once

#ifndef __OUTSTREAMWITHCRC_H
#define __OUTSTREAMWITHCRC_H

#include "Common/CRC.h"
#include "Interface/IInOutStreams.h"

//////////////////////////////////////
// COutStreamWithCRC

class COutStreamWithCRC: 
  public ISequentialOutStream,
  public CComObjectRoot
{
public:
BEGIN_COM_MAP(COutStreamWithCRC)
  COM_INTERFACE_ENTRY(ISequentialOutStream)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(COutStreamWithCRC)

DECLARE_NO_REGISTRY()

  STDMETHOD(Write)(const void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(WritePart)(const void *data, UINT32 size, UINT32 *processedSize);
private:
  CCRC _crc;
  CComPtr<ISequentialOutStream> _stream;
public:
  void Init(ISequentialOutStream *stream)
  {
    _stream = stream;
    _crc.Init();
  }
  void ReleaseStream()
    { _stream.Release(); }
  UINT32 GetCRC() const { return _crc.GetDigest(); }
  void InitCRC() { _crc.Init(); }

};

#endif