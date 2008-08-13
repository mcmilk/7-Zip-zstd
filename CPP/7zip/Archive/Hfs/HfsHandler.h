// HfsHandler.h

#ifndef __ARCHIVE_HFS_HANDLER_H
#define __ARCHIVE_HFS_HANDLER_H

#include "Common/MyCom.h"
#include "../IArchive.h"
#include "HfsIn.h"

namespace NArchive {
namespace NHfs {

class CHandler:
  public IInArchive,
  public CMyUnknownImp
{
  CMyComPtr<IInStream> _stream;
  CDatabase _db;
public:
  MY_UNKNOWN_IMP1(IInArchive)
  INTERFACE_IInArchive(;)
};

}}

#endif
