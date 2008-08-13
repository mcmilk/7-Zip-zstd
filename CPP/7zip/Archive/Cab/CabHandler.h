// CabHandler.h

#ifndef __CAB_HANDLER_H
#define __CAB_HANDLER_H

#include "Common/MyCom.h"
#include "../IArchive.h"
#include "CabIn.h"

namespace NArchive {
namespace NCab {

class CHandler:
  public IInArchive,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP1(IInArchive)

  INTERFACE_IInArchive(;)

private:
  CMvDatabaseEx m_Database;
};

}}

#endif
