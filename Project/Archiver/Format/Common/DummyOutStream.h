// DummyOutStream.h

#pragma once

#ifndef __DUMMYOUTSTREAM_H
#define __DUMMYOUTSTREAM_H

#include "Interface/IInOutStreams.h"

//////////////////////////////////////
// CDummyOutStream

class CDummyOutStream: 
  public ISequentialOutStream,
  public CComObjectRoot
{
public:
BEGIN_COM_MAP(CDummyOutStream)
  COM_INTERFACE_ENTRY(ISequentialOutStream)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CDummyOutStream)

DECLARE_NO_REGISTRY()

  STDMETHOD(Write)(const void *aData, UINT32 aSize, UINT32 *aProcessedSize);
  STDMETHOD(WritePart)(const void *aData, UINT32 aSize, UINT32 *aProcessedSize);
private:
  CComPtr<ISequentialOutStream> m_Stream;
public:
  void Init(ISequentialOutStream *aStream);
};

#endif