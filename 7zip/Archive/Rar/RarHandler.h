// Rar/Handler.h

#ifndef __RAR_HANDLER_H
#define __RAR_HANDLER_H

#include "../IArchive.h"
#include "RarIn.h"
#include "RarVolumeInStream.h"

namespace NArchive {
namespace NRar {

class CHandler: 
  public IInArchive,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP
  
  STDMETHOD(Open)(IInStream *aStream, 
      const UInt64 *aMaxCheckStartPosition,
      IArchiveOpenCallback *anOpenArchiveCallback);  
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

private:
  CRecordVector<CRefItem> _refItems;
  CObjectVector<CItemEx> _items;
  CObjectVector<CInArchive> _archives;
  NArchive::NRar::CInArchiveInfo _archiveInfo;

  UInt64 GetPackSize(int refIndex) const;
  // NArchive::NRar::CInArchive _archive;

  bool IsSolid(int refIndex)
  {
    const CItemEx &item = _items[_refItems[refIndex].ItemIndex];
    if (item.UnPackVersion < 20)
    {
      if (_archiveInfo.IsSolid())
        return (refIndex > 0);
      return false;
    }
    return item.IsSolid();
  }
};

}}

#endif
