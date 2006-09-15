// OutStreamWithCRC.h

#ifndef __OUTSTREAMWITHCRC_H
#define __OUTSTREAMWITHCRC_H

#include "../../../Common/CRC.h"
#include "../../../Common/MyCom.h"
#include "../../IStream.h"

class COutStreamWithCRC: 
  public ISequentialOutStream,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP

  STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);
private:
  CMyComPtr<ISequentialOutStream> _stream;
  UInt64 _size;
  CCRC _crc;
  bool _calculateCrc;
public:
  void SetStream(ISequentialOutStream *stream) { _stream = stream; }
  void Init(bool calculateCrc = true)
  {
    _size = 0;
    _calculateCrc = calculateCrc;
    _crc.Init();
  }
  void ReleaseStream() { _stream.Release(); }
  UInt64 GetSize() const { return _size; }
  UInt32 GetCRC() const { return _crc.GetDigest(); }
  void InitCRC() { _crc.Init(); }
};

#endif
