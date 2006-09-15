// LimitedStreams.h

#ifndef __LIMITEDSTREAMS_H
#define __LIMITEDSTREAMS_H

#include "../../Common/MyCom.h"
#include "../IStream.h"

class CLimitedSequentialInStream: 
  public ISequentialInStream,
  public CMyUnknownImp
{
  CMyComPtr<ISequentialInStream> _stream;
  UInt64 _size;
  UInt64 _pos;
  bool _wasFinished;
public:
  void SetStream(ISequentialInStream *stream) { _stream = stream; }
  void Init(UInt64 streamSize)  
  { 
    _size = streamSize; 
    _pos = 0; 
    _wasFinished = false; 
  }
 
  MY_UNKNOWN_IMP

  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);
  UInt64 GetSize() const { return _pos; }
  bool WasFinished() const { return _wasFinished; }
};

#endif
