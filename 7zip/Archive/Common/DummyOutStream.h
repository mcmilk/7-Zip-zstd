// DummyOutStream.h

#pragma once

#ifndef __DUMMYOUTSTREAM_H
#define __DUMMYOUTSTREAM_H

#include "../../IStream.h"
#include "Common/MyCom.h"

class CDummyOutStream: 
  public ISequentialOutStream,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP

  STDMETHOD(Write)(const void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(WritePart)(const void *data, UINT32 size, UINT32 *processedSize);
private:
  CMyComPtr<ISequentialOutStream> m_Stream;
public:
  void Init(ISequentialOutStream *outStream);
};

#endif
