// RegistryInfo.h

#ifndef __7Z_REGISTRYINFO_H
#define __7Z_REGISTRYINFO_H

#include "MethodInfo.h"

#include "Common/String.h"

#include "Common/Vector.h"

namespace NArchive {
namespace N7z {
namespace NRegistryInfo {

struct CMethodInfo
{
  CSysString Name;
  bool EncoderIsAssigned;
  bool EncoderPropertiesIsAssigned;
  bool DecoderIsAssigned;
  bool DecoderPropertiesIsAssigned;
  UINT32 NumInStreams;
  UINT32 NumOutStreams;
  CLSID Encoder;
  CLSID Decoder;
  CLSID EncoderProperties;
  CLSID DecoderProperties;
  CSysString Description;
};

struct CMethodInfo2: public CMethodInfo
{
  CMethodID MethodID;
};

bool GetMethodInfo(const CMethodID &aMethodID, CMethodInfo &aMethodInfo);
bool EnumerateAllMethods(CObjectVector<CMethodInfo2> &aMethodInfoVector);

struct CMatchFinderInfo
{
  CLSID ClassID;
};

bool GetMatchFinder(const CSysString &aName, CMatchFinderInfo &aMatchFinderInfo);

struct CMethodToCLSIDPair
{
  CMethodID MethodID;
  CLSID ClassID;
};

class CMethodToCLSIDMap
{
  CObjectVector<CMethodToCLSIDPair> m_Pairs;
public:
  CMethodToCLSIDMap() {}
  bool GetCLSID(const CMethodID &aMethodID, CLSID &aCLSID);
  bool GetCLSIDAlways(const CMethodID &aMethodID, CLSID &aCLSID);
};

}}}

#endif
