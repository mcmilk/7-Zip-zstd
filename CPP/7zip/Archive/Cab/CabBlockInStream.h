// CabBlockInStream.cpp

#ifndef __CABBLOCKINSTREAM_H
#define __CABBLOCKINSTREAM_H

#include "Common/MyCom.h"
#include "../../IStream.h"

namespace NArchive {
namespace NCab {

class CCabBlockInStream: 
  public ISequentialInStream,
  public CMyUnknownImp
{
  CMyComPtr<ISequentialInStream> _stream;
  Byte *_buffer;
  UInt32 _pos;
  UInt32 _size;

public:
  UInt32 TotalPackSize;
  UInt32 ReservedSize;
  bool DataError;
  bool MsZip;

  CCabBlockInStream(): _buffer(0), ReservedSize(0), MsZip(false), DataError(false), TotalPackSize(0) {}
  ~CCabBlockInStream();
  bool Create();
  void SetStream(ISequentialInStream *stream) {  _stream = stream; }

  void InitForNewFolder() { TotalPackSize = 0; }
  void InitForNewBlock() { _size = 0; }

  MY_UNKNOWN_IMP

  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);

  HRESULT PreRead(UInt32 &packSize, UInt32 &unpackSize);
};

}}

#endif
