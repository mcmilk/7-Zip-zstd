// WimHandler.h

#ifndef __ARCHIVE_WIM_HANDLER_H
#define __ARCHIVE_WIM_HANDLER_H

#include "Common/MyCom.h"
#include "Common/MyXml.h"

#include "../IArchive.h"
#include "WimIn.h"

namespace NArchive {
namespace NWim {

struct CVolume
{
  CHeader Header;
  CMyComPtr<IInStream> Stream;
};

struct CImageInfo
{
  bool CTimeDefined;
  bool MTimeDefined;
  bool NameDefined;
  // bool IndexDefined;
  
  FILETIME CTime;
  FILETIME MTime;
  UString Name;
  // UInt32 Index;
  
  CImageInfo(): CTimeDefined(false), MTimeDefined(false), NameDefined(false)
      // , IndexDefined(false)
      {}
  void Parse(const CXmlItem &item);
};

struct CXml
{
  CByteBuffer Data;
  UInt16 VolIndex;

  CObjectVector<CImageInfo> Images;

  void Parse();
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
