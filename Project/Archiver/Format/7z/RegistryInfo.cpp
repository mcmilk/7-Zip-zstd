// RegistryInfo.cpp

#include "StdAfx.h"

#include "RegistryInfo.h"

#include "Common/StringConvert.h"
#include "Windows/Registry.h"
#include "Windows/COM.h"

using namespace NWindows;
using namespace NRegistry;

namespace NArchive {
namespace N7z {
namespace NRegistryInfo {

static LPCTSTR kCoderPath = _T("Software\\7-Zip\\Coder");
static LPCTSTR kMatchFinderPath = _T("Software\\7-Zip\\MatchFinder");

static LPCTSTR kDecoderValueName = _T("Decoder");
static LPCTSTR kDecoderPropertiesValueName = _T("DecoderProperties");
static LPCTSTR kEncoderValueName = _T("Encoder");
static LPCTSTR kEncoderPropertiesValueName = _T("EncoderProperties");
static LPCTSTR kDescriptionValueName = _T("Description");
static LPCTSTR kInStreamsValueName = _T("InStreams");
static LPCTSTR kOutStreamsValueName = _T("OutStreams");
static LPCTSTR kCryptoValueName = _T("Crypto");

void MyReadCLSID(CKey &key, LPCTSTR valueName, bool &itemIsAssigned, CLSID &clsID)
{
  itemIsAssigned = false;
  CSysString value;
  if (key.QueryValue(valueName, value) != ERROR_SUCCESS)
    return;
  if(NCOM::StringToGUID(value, clsID) != NOERROR)
    return;
  itemIsAssigned = true;
}

bool ReadMethodInfo(HKEY parentKey, LPCTSTR keyName, CMethodInfo &methodInfo)
{
  methodInfo.Name.Empty();
  methodInfo.DecoderIsAssigned = false;
  methodInfo.EncoderIsAssigned = false;
  methodInfo.Description.Empty();

  CKey key;
  if (key.Open(parentKey, keyName, KEY_READ) != ERROR_SUCCESS)
    return false;

  key.QueryValue(NULL, methodInfo.Name);

  MyReadCLSID(key, kDecoderValueName, methodInfo.DecoderIsAssigned, methodInfo.Decoder);
  MyReadCLSID(key, kDecoderPropertiesValueName, methodInfo.DecoderPropertiesIsAssigned, methodInfo.DecoderProperties);
  MyReadCLSID(key, kEncoderValueName, methodInfo.EncoderIsAssigned, methodInfo.Encoder);
  MyReadCLSID(key, kEncoderPropertiesValueName, methodInfo.EncoderPropertiesIsAssigned, methodInfo.EncoderProperties);

  key.QueryValue(kDescriptionValueName, methodInfo.Description);
  if (key.QueryValue(kInStreamsValueName, methodInfo.NumInStreams) != ERROR_SUCCESS)
    methodInfo.NumInStreams = 1;
  if (key.QueryValue(kOutStreamsValueName, methodInfo.NumOutStreams) != ERROR_SUCCESS)
    methodInfo.NumOutStreams = 1;
  /*
  if (key.QueryValue(kCryptoValueName, methodInfo.Crypto) != ERROR_SUCCESS)
    methodInfo.Crypto = false;
  */
  return true;
}

bool GetMethodInfo(const CMethodID &methodID, CMethodInfo &methodInfo)
{
  CSysString keyName = kCoderPath;
  keyName += "\\";
  keyName += GetSystemString(methodID.ConvertToString());
  return ReadMethodInfo(HKEY_LOCAL_MACHINE, keyName, methodInfo);
}

bool EnumerateAllMethods(CObjectVector<CMethodInfo2> &methodInfoVector)
{
  CSysString keyName = kCoderPath;

  CKey key;
  if (key.Open(HKEY_LOCAL_MACHINE, keyName, KEY_READ) != ERROR_SUCCESS)
    return false;

  CSysStringVector idStrings;
  key.EnumKeys(idStrings);
  for(int i = 0; i < idStrings.Size(); i++)
  {
    CMethodInfo2 methodInfo;
    if (!methodInfo.MethodID.ConvertFromString(idStrings[i]))
      continue;
    if (ReadMethodInfo(key, idStrings[i], methodInfo))
      methodInfoVector.Add(methodInfo);
  }

  return true;
}

bool GetMatchFinder(const CSysString &name, CMatchFinderInfo &matchFinderInfo)
{
  CSysString keyName = kMatchFinderPath;
  keyName += "\\";
  keyName += name;

  CKey key;
  if (key.Open(HKEY_LOCAL_MACHINE, keyName, KEY_READ) != ERROR_SUCCESS)
    return false;

  CSysString value;
  if (key.QueryValue(NULL, value) != ERROR_SUCCESS)
    return false;

  if(NCOM::StringToGUID(value, matchFinderInfo.ClassID) != NOERROR)
    return false;
  return true;
}


bool CMethodToCLSIDMap::GetMethodInfo2(const CMethodID &methodID, CMethodInfo &methodInfo)
{
  for(int i = 0; i < m_Pairs.Size(); i++)
  {
    CMethodToCLSIDPair pair = m_Pairs[i];
    if (pair.MethodID == methodID)
    {
      methodInfo = pair.MethodInfo;
      return true;
    }
  }
  return false;
}

bool CMethodToCLSIDMap::GetMethodInfoAlways(const CMethodID &methodID, CMethodInfo &methodInfo)
{
  if (GetMethodInfo2(methodID, methodInfo))
    return true;
  return GetMethodInfo(methodID, methodInfo);
}

bool CMethodToCLSIDMap::GetCLSID(const CMethodID &methodID, CLSID &clsID)
{
  CMethodInfo methodInfo;
  if (!GetMethodInfo2(methodID, methodInfo))
    return false;
  if (!methodInfo.DecoderIsAssigned)
    return false;
  clsID = methodInfo.Decoder;
  return true;
}

bool CMethodToCLSIDMap::GetCLSIDAlways(const CMethodID &methodID, CLSID &clsID)
{
  CMethodInfo methodInfo;
  if (!GetMethodInfoAlways(methodID, methodInfo))
    return false;
  if (!methodInfo.DecoderIsAssigned)
    return false;
  clsID = methodInfo.Decoder;
  return true;
}


}}}


