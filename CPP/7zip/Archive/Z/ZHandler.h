// ZHandler.h

#ifndef __Z_HANDLER_H
#define __Z_HANDLER_H

#include "Common/MyCom.h"
#include "../IArchive.h"

namespace NArchive {
namespace NZ {

class CHandler: 
  public IInArchive,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP1(IInArchive)
  INTERFACE_IInArchive(;)

private:
  CMyComPtr<IInStream> _stream;
  UInt64 _streamStartPosition;
  UInt64 _packSize;
  Byte _properties;
};

}}

#endif
