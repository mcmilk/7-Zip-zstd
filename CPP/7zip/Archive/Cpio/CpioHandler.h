// Archive/cpio/Handler.h

#ifndef __ARCHIVE_CPIO_HANDLER_H
#define __ARCHIVE_CPIO_HANDLER_H

#include "Common/MyCom.h"
#include "../IArchive.h"

#include "CpioItem.h"

namespace NArchive {
namespace NCpio {

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
