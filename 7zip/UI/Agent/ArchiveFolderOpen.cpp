// Zip/ArchiveFolder.cpp

#include "StdAfx.h"

#include "Agent.h"

#include "Common/StringConvert.h"

#include "../Common/OpenArchive.h"

static const UINT64 kMaxCheckStartPosition = 1 << 20;

static inline UINT GetCurrentFileCodePage()
  {  return AreFileApisANSI() ? CP_ACP : CP_OEMCP; }

void CArchiveFolderManager::LoadFormats()
{
  if (!_formatsLoaded)
    ReadArchiverInfoList(_formats);
}

int CArchiveFolderManager::FindFormat(const UString &type)
{
  // LoadFormats();
  for (int i = 0; i < _formats.Size(); i++)
    if (type.CompareNoCase(_formats[i].Name) == 0)
      return i;
  return -1;
}

STDMETHODIMP CArchiveFolderManager::OpenFolderFile(const wchar_t *filePath, 
    IFolderFolder **resultFolder, IProgress *progress)
{
  CMyComPtr<IArchiveOpenCallback> openArchiveCallback;
  if (progress != 0)
  {
    CMyComPtr<IProgress> progressWrapper = progress;
    progressWrapper.QueryInterface(IID_IArchiveOpenCallback, &openArchiveCallback);
  }
  CAgent *agent = new CAgent();
  CComPtr<IInFolderArchive> archive = agent;
  RINOK(agent->Open(filePath, NULL, openArchiveCallback));
  return agent->BindToRootFolder(resultFolder);
}

/*
HRESULT CAgent::FolderReOpen(
    IArchiveOpenCallback *openArchiveCallback)
{
  return ReOpenArchive(_archive, _archiveFilePath);
}
*/

STDMETHODIMP CArchiveFolderManager::GetTypes(BSTR *types)
{
  LoadFormats();
  UString typesStrings;
  for(int i = 0; i < _formats.Size(); i++)
  {
    if (i != 0)
      typesStrings += L' ';
    typesStrings += _formats[i].Name;
  }
  CMyComBSTR valueTemp = typesStrings;
  *types = valueTemp.Detach();
  return S_OK;
}

STDMETHODIMP CArchiveFolderManager::GetExtension(const wchar_t *type, BSTR *extension)
{
  *extension = 0;
  int formatIndex = FindFormat(type);
  if (formatIndex <  0)
    return E_INVALIDARG;
  // CMyComBSTR valueTemp = _formats[formatIndex].GetAllExtensions();
  CMyComBSTR valueTemp = _formats[formatIndex].Extensions[0].Extension;
  *extension = valueTemp.Detach();
  return S_OK;
}

STDMETHODIMP CArchiveFolderManager::GetIconPath(const wchar_t *type, BSTR *iconPath)
{
  *iconPath = 0;
  int formatIndex = FindFormat(type);
  if (formatIndex <  0)
    return E_INVALIDARG;
  CMyComBSTR iconPathTemp = _formats[formatIndex].FilePath;
  *iconPath = iconPathTemp.Detach();
  return S_OK;
}

STDMETHODIMP CArchiveFolderManager::CreateFolderFile(const wchar_t *type, const wchar_t *filePath, IProgress *progress)
{
  return E_NOTIMPL;
}



