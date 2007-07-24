// WimHandler.h

#ifndef __ARCHIVE_WIM_HANDLER_H
#define __ARCHIVE_WIM_HANDLER_H

#include "Common/MyCom.h"
#include "../IArchive.h"
#include "WimIn.h"

namespace NArchive {
namespace NWim {

struct CVolume
{
  CHeader Header;
  CMyComPtr<IInStream> Stream;
};

struct CXml
{
  CByteBuffer Data;
  UInt16 VolIndex;
};

class CHandler: 
  public IInArchive,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP1(IInArchive)
  INTERFACE_IInArchive(;)

private:
  CDatabase m_Database;
  CObjectVector<CVolume> m_Volumes;
  CObjectVector<CXml> m_Xmls;
  int m_NameLenForStreams;
};

}}

#endif
