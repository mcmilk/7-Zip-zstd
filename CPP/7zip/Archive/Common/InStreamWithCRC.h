// InStreamWithCRC.h

#ifndef __INSTREAMWITHCRC_H
#define __INSTREAMWITHCRC_H

#include "../../../Common/CRC.h"
#include "../../../Common/MyCom.h"
#include "../../IStream.h"

class CSequentialInStreamWithCRC: 
  public ISequentialInStream,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP

  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);
private:
  CMyComPtr<ISequentialInStream> _stream;
  UInt64 _size;
  CCRC _crc;
  bool _wasFinished;
public:
  void SetStream(ISequentialInStream *stream) { _stream = stream;  }
  void Init()
  {
    _size = 0;
    _wasFinished = false;
    _crc.Init();
  }
  void ReleaseStream() { _stream.Release(); }
  UInt32 GetCRC() const { return _crc.GetDigest(); }
  UInt64 GetSize() const { return _size; }
  bool WasFinished() const { return _wasFinished; }
};

class CInStreamWithCRC: 
  public IInStream,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP1(IInStream)

  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);
  STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition);
private:
  CMyComPtr<IInStream> _stream;
  UInt64 _size;
  CCRC _crc;
  bool _wasFinished;
public:
  void SetStream(IInStream *stream) { _stream = stream;  }
  void Init()
  {
    _size = 0;
    _wasFinished = false;
    _crc.Init();
  }
  void ReleaseStream() { _stream.Release(); }
  UInt32 GetCRC() const { return _crc.GetDigest(); }
  UInt64 GetSize() const { return _size; }
  bool WasFinished() const { return _wasFinished; }
};

#endif
