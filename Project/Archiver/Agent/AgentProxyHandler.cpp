// ProxyHandler.cpp

#include "StdAfx.h"

#include "AgentProxyHandler.h"

#include "Windows/PropVariant.h"
#include "Windows/Defs.h"

using namespace std;
using namespace NWindows;

int CFolderItem::FindDirSubItemIndex(const UString &name, int &insertPos) const
{
  int left = 0, right = FolderSubItems.Size();
  while(true)
  {
    if (left == right)
    {
      insertPos = left;
      return -1;
    }
    int mid = (left + right) / 2;
    int compare = name.CollateNoCase(FolderSubItems[mid].Name);
    if (compare == 0)
      return mid;
    if (compare < 0)
      right = mid;
    else
      left = mid + 1;
  }
}

int CFolderItem::FindDirSubItemIndex(const UString &name) const
{
  int insertPos;
  return FindDirSubItemIndex(name, insertPos);
}

void CFolderItem::AddFileSubItem(UINT32 index, const UString &name)
{
  CFileItem aFolderFileItem;
  FileSubItems.Add(aFolderFileItem);
  FileSubItems.Back().Name = name;
  FileSubItems.Back().Index = index;
}

CFolderItem* CFolderItem::AddDirSubItem(UINT32 index, bool leaf, 
    const UString &name)
{
  int insertPos;
  int folderIndex = FindDirSubItemIndex(name, insertPos);
  if (folderIndex >= 0)
  {
    CFolderItem *item = &FolderSubItems[folderIndex];
    if(leaf)
    {
      item->Index = index;
      item->IsLeaf = true;
    }
    return item;
  }
  FolderSubItems.Insert(insertPos, CFolderItem());
  CFolderItem *item = &FolderSubItems[insertPos];
  item->Name = name;
  item->Index = index;
  item->Parent = this;
  item->IsLeaf = leaf;
  return item;
}

void CFolderItem::Clear()
{
  FolderSubItems.Clear();
  FileSubItems.Clear();
}

///////////////////////////////////////////////
// CAgentProxyHandler

void CAgentProxyHandler::ClearState()
{
  // m_HandlerProperties.Clear();
  // m_InternalProperties.Clear();
  FolderItemHead.Clear();
}

HRESULT CAgentProxyHandler::ReInit(IProgress *progress)
{
  ClearState();
  // RETURN_IF_NOT_S_OK(ReadProperties(_archiveHandler));

  // OutputDebugString("before ReadObjects\n");
  // return ReadObjects(_archiveHandler, progress);
  return ReadObjects(Archive, progress);
  // OutputDebugString("after ReadObjects\n");
}


HRESULT CAgentProxyHandler::Init(IInArchive *archiveHandler, 
    const UString &itemDefaultName, 
    const FILETIME &defaultTime,
    UINT32 defaultAttributes,
    IProgress *progress)
{
  ItemDefaultName = itemDefaultName;
  DefaultTime = defaultTime;
  DefaultAttributes = defaultAttributes;

  Archive = archiveHandler;
  return ReInit(progress);
}

/*
HRESULT CAgentProxyHandler::ReadProperties(IArchiveHandler200 *archiveHandler)
{
  CComPtr<IEnumSTATPROPSTG> anEnumProperty;
  RETURN_IF_NOT_S_OK(archiveHandler->EnumProperties(&anEnumProperty));

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
    const CArchiveItemProperty &handlerProperty = m_HandlerProperties[i];
    CArchiveItemProperty internalProperty = handlerProperty;

    if (handlerProperty.ID == kpidPath)
      internalProperty.ID = kpidName;
    m_InternalProperties.Add(internalProperty);
  }
  CArchiveItemProperty internalProperty;
  internalProperty.ID = kpidHandlerItemIndex;
  // internalProperty.Name not defined;
  
  internalProperty.Type = VT_I4;
  m_InternalProperties.Add(internalProperty);
  
  return S_OK;
}
*/

HRESULT CAgentProxyHandler::ReadObjects(IInArchive *archiveHandler, IProgress *progress)
{
  UINT32 numItems;
  RETURN_IF_NOT_S_OK(archiveHandler->GetNumberOfItems(&numItems));
  for(UINT32 itemIndex = 0; itemIndex < numItems; itemIndex++)
  {
    if (progress != NULL)
    {
      UINT64 currentItemIndex = itemIndex; 
      RETURN_IF_NOT_S_OK(progress->SetCompleted(&currentItemIndex));
    }
    NCOM::CPropVariant propVariantPath;
    RETURN_IF_NOT_S_OK(archiveHandler->GetProperty(itemIndex, kpidPath, &propVariantPath));
    CFolderItem *currentItem = &FolderItemHead;
    UString fileName;
    if(propVariantPath.vt == VT_EMPTY)
      fileName = ItemDefaultName;
    else
    {
      if(propVariantPath.vt != VT_BSTR)
        return E_FAIL;
      UString filePath = propVariantPath.bstrVal;

      int len = filePath.Length();
      for (int i = 0; i < len; i++)
      {
        wchar_t c = filePath[i];
        if (c == '\\' || c == '/')
        {
          currentItem = currentItem->AddDirSubItem(-1, false, fileName);
          fileName.Empty();
        }
        else
          fileName += c;
      }
    }

    NCOM::CPropVariant propVariantIsFolder;
    RETURN_IF_NOT_S_OK(archiveHandler->GetProperty(itemIndex, 
        kpidIsFolder, &propVariantIsFolder));
    if(propVariantIsFolder.vt != VT_BOOL)
      return E_FAIL;
    if(VARIANT_BOOLToBool(propVariantIsFolder.boolVal))
      currentItem->AddDirSubItem(itemIndex, true, fileName);
    else
      currentItem->AddFileSubItem(itemIndex, fileName);
  }
  return S_OK;
}

void CAgentProxyHandler::AddRealIndices(const CFolderItem &item, 
    CUIntVector &realIndices)
{
  if (item.IsLeaf)
    realIndices.Add(item.Index);
  for(int i = 0; i < item.FolderSubItems.Size(); i++)
    AddRealIndices(item.FolderSubItems[i], realIndices);
  for(i = 0; i < item.FileSubItems.Size(); i++)
    realIndices.Add(item.FileSubItems[i].Index);
}


void CAgentProxyHandler::GetRealIndices(const CFolderItem &item, 
    const UINT32 *indices, UINT32 numItems, CUIntVector &realIndices)
{
  realIndices.Clear();
  for(int i = 0; i < numItems; i++)
  {
    int index = indices[i];
    int numDirItems = item.FolderSubItems.Size();
    if (index < numDirItems)
      AddRealIndices(item.FolderSubItems[index], realIndices);
    else
      realIndices.Add(item.FileSubItems[index - numDirItems].Index);
  }
  realIndices.Sort();
  // std::sort(realIndices.begin(), realIndices.end());
}

