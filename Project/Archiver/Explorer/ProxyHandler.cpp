// ProxyHandler.cpp

#include "StdAfx.h"

#include "resource.h"
#include "ProxyHandler.h"
#include "Windows/Synchronization.h"
#include "Windows/ResourceString.h"
#include "Common/StringConvert.h"

using namespace NWindows;

static NSynchronization::CCriticalSection g_ColumnsInfoCriticalSection;


CProxyHandlerSpec::CProxyHandlerSpec()
{
  m_FileTypePropertyCaption = MyLoadString(IDS_PROPERTY_FILE_TYPE);
}

CProxyHandlerSpec::~CProxyHandlerSpec()
{
  m_ZipRegistryManager.SaveListViewInfo(m_ClassID, m_ListViewInfo);
}

void CProxyHandlerSpec::SaveColumnsInfo(const NZipSettings::CListViewInfo &CListViewInfo)
{
  NSynchronization::CSingleLock aLock(&g_ColumnsInfoCriticalSection, true);
  m_ListViewInfo = CListViewInfo;
};

void CProxyHandlerSpec::ReadColumnsInfo(NZipSettings::CListViewInfo &aViewInfo)
{
  NSynchronization::CSingleLock aLock(&g_ColumnsInfoCriticalSection, true);
  aViewInfo = m_ListViewInfo;
}

bool CProxyHandlerSpec::Init(IArchiveHandler100 *aHandler, const CLSID &aClassID)
{
  m_ClassID = aClassID;
  m_ZipRegistryManager.ReadListViewInfo(m_ClassID, m_ListViewInfo);
  return ReadProperties(aHandler);
}


struct CPropertyIDNamePair
{
  PROPID PropID;
  UINT ResourceID; // wchar_t *Name;
};

static CPropertyIDNamePair kPropertyIDNamePairs[] =  
{
  { kaipidName, IDS_PROPERTY_NAME },
  // { kaipidPath, L"Path" },
  // { kaipidExtension, L"Extension" },
  // { kaipidIsFolder, L"Is Folder" },
  { kaipidSize, IDS_PROPERTY_SIZE },
  { kaipidPackedSize, IDS_PROPERTY_PACKED_SIZE },
  { kaipidAttributes, IDS_PROPERTY_ATTRIBUTES },
  { kaipidCreationTime, IDS_PROPERTY_CREATION_TIME },
  { kaipidLastAccessTime, IDS_PROPERTY_LAST_ACCESS_TIME },
  { kaipidLastWriteTime, IDS_PROPERTY_LAST_WRITE_TIME },
  { kaipidSolid, IDS_PROPERTY_SOLID },
  { kaipidComment, IDS_PROPERTY_C0MMENT },
  { kaipidEncrypted, IDS_PROPERTY_ENCRYPTED },
  { kaipidDictionarySize, IDS_PROPERTY_DICTIONARY_SIZE },

  { kaipidSplitBefore, IDS_PROPERTY_SPLIT_BEFORE },
  { kaipidSplitAfter, IDS_PROPERTY_SPLIT_AFTER },
  { kaipidCRC, IDS_PROPERTY_CRC }
  // { kaipidType, L"Type" }
};

static const kNumPropertyIDNamePairs = sizeof(kPropertyIDNamePairs) /  
    sizeof(kPropertyIDNamePairs[0]);

static bool GetDefaultPropertyName(PROPID aPropID, CSysString &aName)
{
  for(int i = 0; i < kNumPropertyIDNamePairs; i++)
  {
    const CPropertyIDNamePair &aPair = kPropertyIDNamePairs[i];
    if(aPair.PropID == aPropID)
    {
      CSysString aString = MyLoadString(aPair.ResourceID);
      aName = aString; 
      return true;
    }
  }
  return false;
}

bool CProxyHandlerSpec::ReadProperties(IArchiveHandler100 *anArchiveHandler)
{
  m_HandlerProperties.Clear();
  m_InternalProperties.Clear();
  m_ColumnsProperties.Clear();

  CComPtr<IEnumSTATPROPSTG> anEnumProperty;
  HRESULT aResult = anArchiveHandler->EnumProperties(&anEnumProperty);
  if(aResult != S_OK)
  {
    // TRACE0("EnumProperties");
    return false;
  }
  STATPROPSTG aSrcProperty;

  while(anEnumProperty->Next(1, &aSrcProperty, NULL) == S_OK)
  {
    CArchiveItemProperty aDestProperty;
    aDestProperty.Type = aSrcProperty.vt;
    aDestProperty.ID = aSrcProperty.propid;
    if(!GetDefaultPropertyName(aSrcProperty.propid, aDestProperty.Name))
    {
      if (aSrcProperty.lpwstrName != NULL)
        aDestProperty.Name = GetSystemString(aSrcProperty.lpwstrName);
      else
        aDestProperty.Name = _T("Error");
    }
    if (aSrcProperty.lpwstrName != NULL)
      CoTaskMemFree(aSrcProperty.lpwstrName);
    // MessageBox(NULL, "m_Properties.Add", "", MB_OK);
    m_HandlerProperties.Add(aDestProperty);
  }
  for(int i = 0; i < m_HandlerProperties.Size(); i++)
  {
    const CArchiveItemProperty &aHandlerProperty = m_HandlerProperties[i];
    CArchiveItemProperty anInternalProperty = aHandlerProperty;

    if (aHandlerProperty.ID == kaipidPath)
    {
      anInternalProperty.ID = kaipidName;
      CSysString aName;
      if(GetDefaultPropertyName(kaipidName, aName))
        anInternalProperty.Name = aName;
    }
    m_InternalProperties.Add(anInternalProperty);
    
    if(aHandlerProperty.ID != kaipidIsFolder)
      m_ColumnsProperties.Add(anInternalProperty);
  }
  /*
  CArchiveItemProperty anInternalProperty;
  anInternalProperty.ID = kaipidHandlerItemIndex;
  // anInternalProperty.Name not defined;
  
  anInternalProperty.Type = VT_I4;
  m_InternalProperties.Add(anInternalProperty);
  */
  
  return true;
}

int CProxyHandlerSpec::FindProperty(PROPID aPropID)
{
  for (int i = 0; i < m_ColumnsProperties.Size(); i++)
    if(m_ColumnsProperties[i].ID == aPropID)
      return i;
  return -1;
}

VARTYPE CProxyHandlerSpec::GetTypeOfProperty(PROPID aPropID)
{
  if(aPropID == kaipidType)
    return VT_BSTR;
  int anIndex = FindProperty(aPropID);
  if (anIndex < 0)
    throw 160338;
  return m_ColumnsProperties[anIndex].Type;
}

CSysString CProxyHandlerSpec::GetNameOfProperty(PROPID aPropID)
{
  if(aPropID == kaipidType)
    return m_FileTypePropertyCaption;
  int anIndex = FindProperty(aPropID);
  if (anIndex < 0)
    throw 160338;
  return m_ColumnsProperties[anIndex].Name;
}
