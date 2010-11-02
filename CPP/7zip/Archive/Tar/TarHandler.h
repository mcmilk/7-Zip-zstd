// TarHandler.h

#ifndef __TAR_HANDLER_H
#define __TAR_HANDLER_H

#include "Common/MyCom.h"
#include "../IArchive.h"

#include "../../Compress/CopyCoder.h"

#include "TarItem.h"

namespace NArchive {
namespace NTar {

class CHandler:
  public IInArchive,
  public IArchiveOpenSeq,
  public IInArchiveGetStream,
  public IOutArchive,
  public CMyUnknownImp
{
  CObjectVector<CItemEx> _items;
  CMyComPtr<IInStream> _stream;
  CMyComPtr<ISequentialInStream> _seqStream;
  
  UInt32 _curIndex;
  bool _latestIsRead;
  CItemEx _latestItem;

  UInt64 _phySize;
  UInt64 _headersSize;
  bool _phySizeDefined;
  AString _errorMessage;

  NCompress::CCopyCoder *copyCoderSpec;
  CMyComPtr<ICompressCoder> copyCoder;

  HRESULT ReadItem2(ISequentialInStream *stream, bool &filled, CItemEx &itemInfo);
  HRESULT Open2(IInStream *stream, IArchiveOpenCallback *callback);
  HRESULT SkipTo(UInt32 index);

public:
  MY_UNKNOWN_IMP4(
    IInArchive,
    IArchiveOpenSeq,
    IInArchiveGetStream,
    IOutArchive
  )

  INTERFACE_IInArchive(;)
  INTERFACE_IOutArchive(;)
  STDMETHOD(OpenSeq)(ISequentialInStream *stream);
  STDMETHOD(GetStream)(UInt32 index, ISequentialInStream **stream);

  CHandler();
};

}}

#endif
