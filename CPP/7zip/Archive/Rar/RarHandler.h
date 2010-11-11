// Rar/Handler.h

#ifndef __RAR_HANDLER_H
#define __RAR_HANDLER_H

#include "../IArchive.h"

#include "../../Common/CreateCoder.h"

#include "RarIn.h"
#include "RarVolumeInStream.h"

namespace NArchive {
namespace NRar {

class CHandler:
  public IInArchive,
  PUBLIC_ISetCompressCodecsInfo
  public CMyUnknownImp
{
  CRecordVector<CRefItem> _refItems;
  CObjectVector<CItemEx> _items;
  CObjectVector<CInArchive> _archives;
  NArchive::NRar::CInArchiveInfo _archiveInfo;
  AString _errorMessage;

  DECL_EXTERNAL_CODECS_VARS

  UInt64 GetPackSize(int refIndex) const;

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
  void AddErrorMessage(const AString &s)
  {
    if (!_errorMessage.IsEmpty())
      _errorMessage += '\n';
    _errorMessage += s;
  }

  HRESULT Open2(IInStream *stream,
      const UInt64 *maxCheckStartPosition,
      IArchiveOpenCallback *openCallback);

public:
  MY_QUERYINTERFACE_BEGIN2(IInArchive)
  QUERY_ENTRY_ISetCompressCodecsInfo
  MY_QUERYINTERFACE_END
  MY_ADDREF_RELEASE
  
  INTERFACE_IInArchive(;)

  DECL_ISetCompressCodecsInfo
};

}}

#endif
