// NSisHandler.h

#ifndef __NSIS_HANDLER_H
#define __NSIS_HANDLER_H

#include "Common/MyCom.h"
#include "../IArchive.h"

#include "NsisIn.h"

namespace NArchive {
namespace NNsis {

class CHandler: 
  public IInArchive,
  public CMyUnknownImp
{
  CMyComPtr<IInStream> _inStream;
  CInArchive _archive;

  bool GetUncompressedSize(int index, UInt32 &size);
  bool GetCompressedSize(int index, UInt32 &size);

public:
  MY_UNKNOWN_IMP1(IInArchive)

  STDMETHOD(Open)(IInStream *stream, const UInt64 *maxCheckStartPosition, IArchiveOpenCallback *openArchiveCallback);  
  STDMETHOD(Close)();  
  STDMETHOD(GetNumberOfItems)(UInt32 *numItems);  
  STDMETHOD(GetProperty)(UInt32 index, PROPID propID, PROPVARIANT *value);
  STDMETHOD(Extract)(const UInt32* indices, UInt32 numItems, Int32 testMode, IArchiveExtractCallback *extractCallback);
  STDMETHOD(GetArchiveProperty)(PROPID propID, PROPVARIANT *value);
  STDMETHOD(GetNumberOfProperties)(UInt32 *numProperties);  
  STDMETHOD(GetPropertyInfo)(UInt32 index, BSTR *name, PROPID *propID, VARTYPE *varType);
  STDMETHOD(GetNumberOfArchiveProperties)(UInt32 *numProperties);  
  STDMETHOD(GetArchivePropertyInfo)(UInt32 index, BSTR *name, PROPID *propID, VARTYPE *varType);
};

}}

#endif
