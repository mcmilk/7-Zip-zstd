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

  INTERFACE_IInArchive(;)

private:
  CObjectVector<CItemEx> _items;
  CMyComPtr<IInStream> _stream;
};

}}

#endif
