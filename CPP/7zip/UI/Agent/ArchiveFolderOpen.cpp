// Zip/ArchiveFolder.cpp

#include "StdAfx.h"

#include "Agent.h"

#include "Common/StringConvert.h"

#include "../Common/OpenArchive.h"

static inline UINT GetCurrentFileCodePage()
  {  return AreFileApisANSI() ? CP_ACP : CP_OEMCP; }

void CArchiveFolderManager::LoadFormats()
{
  if (!_codecs)
  {
    _compressCodecsInfo = _codecs = new CCodecs;
    _codecs->Load();
  }
}

int CArchiveFolderManager::FindFormat(const UString &type)
{
  for (int i = 0; i < _codecs->Formats.Size(); i++)
    if (type.CompareNoCase(_codecs->Formats[i].Name) == 0)
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
  CMyComPtr<IInFolderArchive> archive = agent;
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


/*
STDMETHODIMP CArchiveFolderManager::GetExtensions(const wchar_t *type, BSTR *extensions)
{
  *extensions = 0;
  int formatIndex = FindFormat(type);
  if (formatIndex <  0)
    return E_INVALIDARG;
  CMyComBSTR valueTemp = _codecs.Formats[formatIndex].GetAllExtensions(); // Exts[0].Ext;
  *extensions = valueTemp.Detach();
  return S_OK;
}
*/
STDMETHODIMP CArchiveFolderManager::GetExtensions(BSTR *extensions)
{
  LoadFormats();
  *extensions = 0;
  UString res;
  for (int i = 0; i < _codecs->Libs.Size(); i++)
  {
    const CCodecLib &lib = _codecs->Libs[i];
    for (int j = 0; j < lib.IconPairs.Size(); j++)
    {
      if (!res.IsEmpty())
        res += L' ';
      res += lib.IconPairs[j].Ext;
    }
  }
  CMyComBSTR valueTemp = res;
  *extensions = valueTemp.Detach();
  return S_OK;
}

STDMETHODIMP CArchiveFolderManager::GetIconPath(const wchar_t *ext, BSTR *iconPath, Int32 *iconIndex)
{
  LoadFormats();
  *iconPath = 0;
  *iconIndex = 0;
  for (int i = 0; i < _codecs->Libs.Size(); i++)
  {
    const CCodecLib &lib = _codecs->Libs[i];
    int ii = lib.FindIconIndex(ext);
    if (ii >= 0)
    {
      CMyComBSTR iconPathTemp = GetUnicodeString(lib.Path, GetCurrentFileCodePage());
      *iconIndex = ii;
      *iconPath = iconPathTemp.Detach();
      return S_OK;
    }
  }
  return S_OK;
}

/*
STDMETHODIMP CArchiveFolderManager::GetTypes(BSTR *types)
{
  LoadFormats();
  UString typesStrings;
  for(int i = 0; i < _codecs.Formats.Size(); i++)
  {
    const CArcInfoEx &ai = _codecs.Formats[i];
    if (ai.AssociateExts.Size() == 0)
      continue;
    if (i != 0)
      typesStrings += L' ';
    typesStrings += ai.Name;
  }
  CMyComBSTR valueTemp = typesStrings;
  *types = valueTemp.Detach();
  return S_OK;
}
STDMETHODIMP CArchiveFolderManager::CreateFolderFile(const wchar_t * type, 
    const wchar_t * filePath, IProgress progress)
{
  return E_NOTIMPL;
}
*/
