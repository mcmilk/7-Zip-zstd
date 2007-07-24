// DebHandler.h

#ifndef __DEB_HANDLER_H
#define __DEB_HANDLER_H

#include "Common/MyCom.h"
#include "../IArchive.h"

#include "DebItem.h"

namespace NArchive {
namespace NDeb {

class CHandler: 
  public IInArchive,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP1(IInArchive)

  INTERFACE_IInArchive(;)

private:
  CObjectVector<CItemEx> _items;
  CMyComPtr<IInStream> _inStream;
};

}}

#endif
