// RPM/Handler.h

#ifndef __RPM_HANDLER_H
#define __RPM_HANDLER_H

#include "Common/MyCom.h"
#include "../IArchive.h"

namespace NArchive {
namespace NRpm {

class CHandler: 
  public IInArchive,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP1(IInArchive)

  INTERFACE_IInArchive(;)

private:
  CMyComPtr<IInStream> m_InStream;
  UInt64 m_Pos;
  UInt64 m_Size;
};

}}

#endif
