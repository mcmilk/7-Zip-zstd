// Rar/Handler.h

#pragma once

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
      const UINT64 *aMaxCheckStartPosition,
      IArchiveOpenCallback *anOpenArchiveCallback);  
  STDMETHOD(Close)();  
  STDMETHOD(GetNumberOfItems)(UINT32 *numItems);  
  STDMETHOD(GetProperty)(UINT32 index, PROPID propID,  PROPVARIANT *value);
  STDMETHOD(Extract)(const UINT32* indices, UINT32 numItems, 
      INT32 testMode, IArchiveExtractCallback *anExtractCallback);

  STDMETHOD(GetArchiveProperty)(PROPID propID, PROPVARIANT *value);

  STDMETHOD(GetNumberOfProperties)(UINT32 *numProperties);  
  STDMETHOD(GetPropertyInfo)(UINT32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType);

  STDMETHOD(GetNumberOfArchiveProperties)(UINT32 *numProperties);  
  STDMETHOD(GetArchivePropertyInfo)(UINT32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType);

private:
  CRecordVector<CRefItem> _refItems;
  CObjectVector<CItemEx> _items;
  CObjectVector<CInArchive> _archives;
  NArchive::NRar::CInArchiveInfo _archiveInfo;

  UINT64 GetPackSize(int refIndex) const;
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