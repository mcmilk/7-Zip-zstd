// RegistryInfo.cpp

#include "StdAfx.h"

#include "RegistryInfo.h"

#include "Windows/Registry.h"
#include "Windows/COM.h"
#include "Common/StringConvert.h"

using namespace NWindows;
using namespace NRegistry;

namespace NArchive {
namespace N7z {
namespace NRegistryInfo {

static LPCTSTR kCoderPath = _T("Software\\7-Zip\\Coder");
static LPCTSTR kMatchFinderPath = _T("Software\\7-Zip\\MatchFinder");

static LPCTSTR aDecoderValueName = _T("Decoder");
static LPCTSTR aDecoderPropertiesValueName = _T("DecoderProperties");
static LPCTSTR anEncoderValueName = _T("Encoder");
static LPCTSTR anEncoderPropertiesValueName = _T("EncoderProperties");
static LPCTSTR aDescriptionValueName = _T("Description");
static LPCTSTR anInStreamsValueName = _T("InStreams");
static LPCTSTR anOutStreamsValueName = _T("OutStreams");

void MyReadCLSID(CKey &aKey, LPCTSTR aValueName, bool &ItemIsAssigned, CLSID &aCLSID)
{
  ItemIsAssigned = false;
  CSysString aString;
  if (aKey.QueryValue(aValueName, aString) != ERROR_SUCCESS)
    return;
  if(NCOM::StringToGUID(aString, aCLSID) != NOERROR)
    return;
  ItemIsAssigned = true;
}

bool ReadMethodInfo(HKEY aKeyParrent, LPCTSTR aKeyName, CMethodInfo &aMethodInfo)
{
  aMethodInfo.Name.Empty();
  aMethodInfo.DecoderIsAssigned = false;
  aMethodInfo.EncoderIsAssigned = false;
  aMethodInfo.Description.Empty();

  CKey aKey;
  if (aKey.Open(aKeyParrent, aKeyName, KEY_READ) != ERROR_SUCCESS)
    return false;

  aKey.QueryValue(NULL, aMethodInfo.Name);

  MyReadCLSID(aKey, aDecoderValueName, aMethodInfo.DecoderIsAssigned, aMethodInfo.Decoder);
  MyReadCLSID(aKey, aDecoderPropertiesValueName, aMethodInfo.DecoderPropertiesIsAssigned, aMethodInfo.DecoderProperties);
  MyReadCLSID(aKey, anEncoderValueName, aMethodInfo.EncoderIsAssigned, aMethodInfo.Encoder);
  MyReadCLSID(aKey, anEncoderPropertiesValueName, aMethodInfo.EncoderPropertiesIsAssigned, aMethodInfo.EncoderProperties);

  aKey.QueryValue(aDescriptionValueName, aMethodInfo.Description);
  if (aKey.QueryValue(anInStreamsValueName, aMethodInfo.NumInStreams) != ERROR_SUCCESS)
    aMethodInfo.NumInStreams = 1;
  if (aKey.QueryValue(anOutStreamsValueName, aMethodInfo.NumOutStreams) != ERROR_SUCCESS)
    aMethodInfo.NumOutStreams = 1;
  return true;
}

bool GetMethodInfo(const CMethodID &aMethodID, CMethodInfo &aMethodInfo)
{
  CSysString aKeyName = kCoderPath;
  aKeyName += "\\";
  aKeyName += GetSystemString(aMethodID.ConvertToString());
  return ReadMethodInfo(HKEY_LOCAL_MACHINE, aKeyName, aMethodInfo);
}

bool EnumerateAllMethods(CObjectVector<CMethodInfo2> &aMethodInfoVector)
{
  CSysString aKeyName = kCoderPath;

  CKey aKey;
  if (aKey.Open(HKEY_LOCAL_MACHINE, aKeyName, KEY_READ) != ERROR_SUCCESS)
    return false;

  CSysStringVector anIDs;
  aKey.EnumKeys(anIDs);
  for(int i = 0; i < anIDs.Size(); i++)
  {
    CMethodInfo2 aMethodInfo;
    if (!aMethodInfo.MethodID.ConvertFromString(anIDs[i]))
      continue;
    if (ReadMethodInfo(aKey, anIDs[i], aMethodInfo))
      aMethodInfoVector.Add(aMethodInfo);
  }

  return true;
}

bool GetMatchFinder(const CSysString &aName, CMatchFinderInfo &aMatchFinderInfo)
{
  CSysString aKeyName = kMatchFinderPath;
  aKeyName += "\\";
  aKeyName += aName;

  CKey aKey;
  if (aKey.Open(HKEY_LOCAL_MACHINE, aKeyName, KEY_READ) != ERROR_SUCCESS)
    return false;

  CSysString aString;
  if (aKey.QueryValue(NULL, aString) != ERROR_SUCCESS)
    return false;

  if(NCOM::StringToGUID(aString, aMatchFinderInfo.ClassID) != NOERROR)
    return false;
  return true;
}


bool CMethodToCLSIDMap::GetCLSID(const CMethodID &aMethodID, CLSID &aCLSID)
{
  for(int i = 0; i < m_Pairs.Size(); i++)
  {
    CMethodToCLSIDPair aPair = m_Pairs[i];
    if (aPair.MethodID == aMethodID)
    {
      aCLSID = aPair.ClassID;
      return true;
    }
  }
  return false;
}

bool CMethodToCLSIDMap::GetCLSIDAlways(const CMethodID &aMethodID, CLSID &aCLSID)
{
  if (GetCLSID(aMethodID, aCLSID))
    return true;
  CMethodInfo aMethodInfo;
  if (!GetMethodInfo(aMethodID, aMethodInfo))
    return false;
  if (!aMethodInfo.DecoderIsAssigned)
    return false;
  aCLSID = aMethodInfo.Decoder;
  return true;
}


}}}


