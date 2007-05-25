// ArjHandler.h

#ifndef __ARJ_HANDLER_H
#define __ARJ_HANDLER_H

#include "Common/MyCom.h"
#include "../IArchive.h"
#include "ArjIn.h"

namespace NArchive {
namespace NArj {

class CHandler: 
  public IInArchive,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP1(IInArchive)

  STDMETHOD(Open)(IInStream *inStream, 
      const UInt64 *maxCheckStartPosition,
      IArchiveOpenCallback *callback);  
  STDMETHOD(Close)();  
  STDMETHOD(GetNumberOfItems)(UInt32 *numItems);  
  STDMETHOD(GetProperty)(UInt32 index, PROPID propID,  PROPVARIANT *value);
  STDMETHOD(Extract)(const UInt32* indices, UInt32 numItems, 
      Int32 testMode, IArchiveExtractCallback *anExtractCallback);

  STDMETHOD(GetArchiveProperty)(PROPID propID, PROPVARIANT *value);

  STDMETHOD(GetNumberOfProperties)(UInt32 *numProperties);  
  STDMETHOD(GetPropertyInfo)(UInt32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType);

  STDMETHOD(GetNumberOfArchiveProperties)(UInt32 *numProperties);  
  STDMETHOD(GetArchivePropertyInfo)(UInt32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType);

  CHandler();
private:
  CObjectVector<CItemEx> _items;
  CMyComPtr<IInStream> _stream;
};

}}

#endif
