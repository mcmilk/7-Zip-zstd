// ProxyHandler.cpp

#include "StdAfx.h"

#include "AgentProxyHandler.h"

#include "Windows/PropVariant.h"
#include "Windows/Defs.h"

using namespace std;
using namespace NWindows;

int CFolderItem::FindDirSubItemIndex(const UString &aName, int &anInsertPos) const
{
  int aLeft = 0, aRight = m_FolderSubItems.Size();
  while(true)
  {
    if (aLeft == aRight)
    {
      anInsertPos = aLeft;
      return -1;
    }
    int aMid = (aLeft + aRight) / 2;
    int aCompare = aName.CollateNoCase(m_FolderSubItems[aMid].m_Name);
    if (aCompare == 0)
      return aMid;
    if (aCompare < 0)
      aRight = aMid;
    else
      aLeft = aMid + 1;
  }
}

int CFolderItem::FindDirSubItemIndex(const UString &aName) const
{
  int anInsertPos;
  return FindDirSubItemIndex(aName, anInsertPos);
}

void CFolderItem::AddFileSubItem(UINT32 anIndex, const UString &aName)
{
  CFileItem aFolderFileItem;
  m_FileSubItems.Add(aFolderFileItem);
  m_FileSubItems.Back().m_Name = aName;
  m_FileSubItems.Back().m_Index = anIndex;
}

CFolderItem* CFolderItem::AddDirSubItem(UINT32 anIndex, bool anLeaf, 
    const UString &aName)
{
  int anInsertPos;
  int aFolderIndex = FindDirSubItemIndex(aName, anInsertPos);
  if (aFolderIndex >= 0)
  {
    CFolderItem *anItem = &m_FolderSubItems[aFolderIndex];
    if(anLeaf)
    {
      anItem->m_Index = anIndex;
      anItem->m_IsLeaf = true;
    }
    return anItem;
  }
  m_FolderSubItems.Insert(anInsertPos, CFolderItem());
  CFolderItem *anItem = &m_FolderSubItems[anInsertPos];
  anItem->m_Name = aName;
  anItem->m_Index = anIndex;
  anItem->m_Parent = this;
  anItem->m_IsLeaf = anLeaf;
  return anItem;
}

void CFolderItem::Clear()
{
  m_FolderSubItems.Clear();
  m_FileSubItems.Clear();
}

///////////////////////////////////////////////
// CAgentProxyHandler

void CAgentProxyHandler::ClearState()
{
  // m_HandlerProperties.Clear();
  // m_InternalProperties.Clear();
  m_FolderItemHead.Clear();
}

HRESULT CAgentProxyHandler::ReInit(IProgress *aProgress)
{
  ClearState();
  // RETURN_IF_NOT_S_OK(ReadProperties(m_ArchiveHandler));

  // OutputDebugString("before ReadObjects\n");
  // return ReadObjects(m_ArchiveHandler, aProgress);
  HRESULT aResult = ReadObjects(m_ArchiveHandler, aProgress);
  // OutputDebugString("after ReadObjects\n");
  return aResult;
}


HRESULT CAgentProxyHandler::Init(IArchiveHandler200 *anArchiveHandler, 
    const UString &anItemDefaultName, 
    const FILETIME &aDefaultTime,
    UINT32 aDefaultAttributes,
    IProgress *aProgress)
{
  // m_ArchiveFileInfo = anArchiveFileInfo;
  m_ItemDefaultName = anItemDefaultName;
  m_DefaultTime = aDefaultTime;
  m_DefaultAttributes = aDefaultAttributes;

  m_ArchiveHandler = anArchiveHandler;
  return ReInit(aProgress);
}

/*
HRESULT CAgentProxyHandler::ReadProperties(IArchiveHandler200 *anArchiveHandler)
{
  CComPtr<IEnumSTATPROPSTG> anEnumProperty;
  RETURN_IF_NOT_S_OK(anArchiveHandler->EnumProperties(&anEnumProperty));

  STATPROPSTG aSrcProperty;

  while(anEnumProperty->Next(1, &aSrcProperty, NULL) == S_OK)
  {
    CArchiveItemProperty aDestProperty;
    aDestProperty.Type = aSrcProperty.vt;
    aDestProperty.ID = aSrcProperty.propid;
    // if(!GetDefaultPropertyName(aSrcProperty.propid, aDestProperty.Name))
    {
      if (aSrcProperty.lpwstrName != NULL)
        aDestProperty.Name = GetSystemString(aSrcProperty.lpwstrName);
      else
        aDestProperty.Name = _T("Error");
    }
    if (aSrcProperty.lpwstrName != NULL)
      CoTaskMemFree(aSrcProperty.lpwstrName);
    m_HandlerProperties.Add(aDestProperty);
  }
  for(int i = 0; i < m_HandlerProperties.Size(); i++)
  {
    const CArchiveItemProperty &aHandlerProperty = m_HandlerProperties[i];
    CArchiveItemProperty anInternalProperty = aHandlerProperty;

    if (aHandlerProperty.ID == kaipidPath)
      anInternalProperty.ID = kaipidName;
    m_InternalProperties.Add(anInternalProperty);
  }
  CArchiveItemProperty anInternalProperty;
  anInternalProperty.ID = kaipidHandlerItemIndex;
  // anInternalProperty.Name not defined;
  
  anInternalProperty.Type = VT_I4;
  m_InternalProperties.Add(anInternalProperty);
  
  return S_OK;
}
*/

HRESULT CAgentProxyHandler::ReadObjects(IArchiveHandler200 *anArchiveHandler, IProgress *aProgress)
{
  UINT32 aNumItems;
  RETURN_IF_NOT_S_OK(anArchiveHandler->GetNumberOfItems(&aNumItems));
  for(UINT32 anItemIndex = 0; anItemIndex < aNumItems; anItemIndex++)
  {
    if (aProgress != NULL)
    {
      UINT64 aCurrentItemIndex = anItemIndex; 
      RETURN_IF_NOT_S_OK(aProgress->SetCompleted(&aCurrentItemIndex));
    }
    NCOM::CPropVariant aPropVariantPath;
    RETURN_IF_NOT_S_OK(anArchiveHandler->GetProperty(anItemIndex, kaipidPath, &aPropVariantPath));
    CFolderItem *aCurrentItem = &m_FolderItemHead;
    UString aFileName;
    if(aPropVariantPath.vt == VT_EMPTY)
      aFileName = m_ItemDefaultName;
    else
    {
      if(aPropVariantPath.vt != VT_BSTR)
        return E_FAIL;
      UString aFilePath = aPropVariantPath.bstrVal;

      int aLen = aFilePath.Length();
      for (int i = 0; i < aLen; i++)
      {
        wchar_t c = aFilePath[i];
        if (c == '\\' || c == '/')
        {
          aCurrentItem = aCurrentItem->AddDirSubItem(-1, false, aFileName);
          aFileName.Empty();
        }
        else
          aFileName += c;
      }
    }

    NCOM::CPropVariant aPropVariantIsFolder;
    RETURN_IF_NOT_S_OK(anArchiveHandler->GetProperty(anItemIndex, 
        kaipidIsFolder, &aPropVariantIsFolder));
    if(aPropVariantIsFolder.vt != VT_BOOL)
      return E_FAIL;
    if(VARIANT_BOOLToBool(aPropVariantIsFolder.boolVal))
      aCurrentItem->AddDirSubItem(anItemIndex, true, aFileName);
    else
      aCurrentItem->AddFileSubItem(anItemIndex, aFileName);
  }
  return S_OK;
}

void CAgentProxyHandler::AddRealIndexes(const CFolderItem &anItem, 
    vector<UINT32> &aRealIndexes)
{
  if (anItem.m_IsLeaf)
    aRealIndexes.push_back(anItem.m_Index);
  for(int i = 0; i < anItem.m_FolderSubItems.Size(); i++)
    AddRealIndexes(anItem.m_FolderSubItems[i], aRealIndexes);
  for(i = 0; i < anItem.m_FileSubItems.Size(); i++)
    aRealIndexes.push_back(anItem.m_FileSubItems[i].m_Index);
}


void CAgentProxyHandler::GetRealIndexes(const CFolderItem &anItem, 
    const UINT32 *anIndexes, 
    UINT32 aNumItems, 
    vector<UINT32> &aRealIndexes)
{
  aRealIndexes.clear();
  for(int i = 0; i < aNumItems; i++)
  {
    int anIndex = anIndexes[i];
    int aNumDirItems = anItem.m_FolderSubItems.Size();
    if (anIndex < aNumDirItems)
      AddRealIndexes(anItem.m_FolderSubItems[anIndex], aRealIndexes);
    else
      aRealIndexes.push_back(anItem.m_FileSubItems[anIndex - aNumDirItems].m_Index);
  }
  std::sort(aRealIndexes.begin(), aRealIndexes.end());
}

