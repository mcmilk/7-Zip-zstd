// ChmHandler.h

#ifndef __ARCHIVE_CHM_HANDLER_H
#define __ARCHIVE_CHM_HANDLER_H

#include "Common/MyCom.h"
#include "../IArchive.h"
#include "ChmIn.h"

namespace NArchive {
namespace NChm {

class CHandler: 
  public IInArchive,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP1(IInArchive)

  INTERFACE_IInArchive(;)

private:
  CFilesDatabase m_Database;
  CMyComPtr<IInStream> m_Stream;
};

}}

#endif
