// Tar/Handler.h

#ifndef __TAR_HANDLER_H
#define __TAR_HANDLER_H

#include "Common/MyCom.h"
#include "../IArchive.h"

#include "TarItem.h"

namespace NArchive {
namespace NTar {

class CHandler: 
  public IInArchive,
  public IOutArchive,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP2(
    IInArchive,
    IOutArchive
  )

  INTERFACE_IInArchive(;)
  INTERFACE_IOutArchive(;)

private:
  CObjectVector<CItemEx> _items;
  CMyComPtr<IInStream> _inStream;
};

}}

#endif
