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

  STDMETHOD(Write)(const void *aData, UINT32 aSize, UINT32 *aProcessedSize);
  STDMETHOD(WritePart)(const void *aData, UINT32 aSize, UINT32 *aProcessedSize);
private:
  CCRC m_Crc;
  CComPtr<ISequentialOutStream> m_Stream;
public:
  void Init(ISequentialOutStream *aStream);
  void ReleaseStream()
    { m_Stream.Release(); }
  UINT32 GetCRC() const { return m_Crc.GetDigest(); }
  void InitCRC();
};

#endif