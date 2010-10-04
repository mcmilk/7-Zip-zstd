// WimHandler.h

#ifndef __ARCHIVE_WIM_HANDLER_H
#define __ARCHIVE_WIM_HANDLER_H

#include "Common/MyCom.h"
#include "Common/MyXml.h"

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

  void ToUnicode(UString &s);
  void Parse();
};


class CHandler:
  public IInArchive,
  public CMyUnknownImp
{
  CDatabase _db;
  UInt32 _version;
  bool _isOldVersion;
  CObjectVector<CVolume> _volumes;
  CObjectVector<CXml> _xmls;
  int _nameLenForStreams;
  bool _xmlInComments;

public:
  MY_UNKNOWN_IMP1(IInArchive)
  INTERFACE_IInArchive(;)
};

class COutHandler:
  public IOutArchive,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP1(IOutArchive)
  INTERFACE_IOutArchive(;)
};

}}

#endif
