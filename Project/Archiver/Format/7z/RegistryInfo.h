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
  // bool Crypto;
};

struct CMethodInfo2: public CMethodInfo
{
  CMethodID MethodID;
};

bool GetMethodInfo(const CMethodID &methodID, CMethodInfo &methodInfo);
bool EnumerateAllMethods(CObjectVector<CMethodInfo2> &methodInfoVector);

struct CMatchFinderInfo
{
  CLSID ClassID;
};

bool GetMatchFinder(const CSysString &name, CMatchFinderInfo &matchFinderInfo);

struct CMethodToCLSIDPair
{
  CMethodID MethodID;
  CMethodInfo MethodInfo;
};

class CMethodToCLSIDMap
{
  CObjectVector<CMethodToCLSIDPair> m_Pairs;
public:
  CMethodToCLSIDMap() {}
  bool GetCLSID(const CMethodID &methodID, CLSID &clsID);
  bool GetCLSIDAlways(const CMethodID &methodID, CLSID &clsID);
  bool GetMethodInfo2(const CMethodID &methodID, CMethodInfo &methodInfo);
  bool GetMethodInfoAlways(const CMethodID &methodID, CMethodInfo &methodInfo);
};

}}}

#endif

