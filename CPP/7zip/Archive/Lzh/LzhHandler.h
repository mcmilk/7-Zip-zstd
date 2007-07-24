// LzhHandler.h

#ifndef __LZH_HANDLER_H
#define __LZH_HANDLER_H

#include "Common/MyCom.h"
#include "../IArchive.h"
#include "LzhIn.h"

namespace NArchive {
namespace NLzh {

class CHandler: 
  public IInArchive,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP1(IInArchive)

  INTERFACE_IInArchive(;)

  CHandler();
private:
  CObjectVector<CItemEx> _items;
  CMyComPtr<IInStream> _stream;
};

}}

#endif
