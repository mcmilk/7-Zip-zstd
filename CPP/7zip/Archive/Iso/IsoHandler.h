// Iso/Handler.h

#ifndef __ISO_HANDLER_H
#define __ISO_HANDLER_H

#include "Common/MyCom.h"
#include "../IArchive.h"

#include "IsoItem.h"
#include "IsoIn.h"

namespace NArchive {
namespace NIso {

class CHandler: 
  public IInArchive,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP1(
    IInArchive
  )

  INTERFACE_IInArchive(;)

private:
  CMyComPtr<IInStream> _inStream;
  CInArchive _archive;
};

}}

#endif
