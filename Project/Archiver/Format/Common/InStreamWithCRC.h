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

  STDMETHOD(Read)(void *aData, UINT32 aSize, UINT32 *aProcessedSize);
  STDMETHOD(ReadPart)(void *aData, UINT32 aSize, UINT32 *aProcessedSize);
  STDMETHOD(Seek)(INT64 anOffset, UINT32 aSeekOrigin, UINT64 *aNewPosition);
private:
  CCRC m_Crc;
  CComPtr<IInStream> m_Stream;
public:
  void Init(IInStream *aStream)
  {
    m_Stream = aStream;
    m_Crc.Init();
  }
  void ReleaseStream()
    { m_Stream.Release(); }
  UINT32 GetCRC() const { return m_Crc.GetDigest(); }
};

#endif