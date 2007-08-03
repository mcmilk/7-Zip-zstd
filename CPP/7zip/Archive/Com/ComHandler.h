// ComHandler.h

#ifndef __ARCHIVE_COM_HANDLER_H
#define __ARCHIVE_COM_HANDLER_H

#include "Common/MyCom.h"
#include "../IArchive.h"
#include "ComIn.h"

namespace NArchive {
namespace NCom {

class CHandler: 
  public IInArchive,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP1(IInArchive)
  INTERFACE_IInArchive(;)

private:
  CMyComPtr<IInStream> _stream;
  CDatabase _db;
};

}}

#endif
