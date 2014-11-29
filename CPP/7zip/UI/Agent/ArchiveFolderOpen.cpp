// Agent/ArchiveFolderOpen.cpp

#include "StdAfx.h"

#include "Agent.h"

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
  FOR_VECTOR (i, _codecs->Formats)
    if (type.IsEqualToNoCase(_codecs->Formats[i].Name))
      return i;
  return -1;
}

STDMETHODIMP CArchiveFolderManager::OpenFolderFile(IInStream *inStream,
    const wchar_t *filePath, const wchar_t *arcFormat,
    IFolderFolder **resultFolder, IProgress *progress)
{
  CMyComPtr<IArchiveOpenCallback> openArchiveCallback;
  if (progress)
  {
    CMyComPtr<IProgress> progressWrapper = progress;
    progressWrapper.QueryInterface(IID_IArchiveOpenCallback, &openArchiveCallback);
  }
  CAgent *agent = new CAgent();
  CMyComPtr<IInFolderArchive> archive = agent;
  RINOK(agent->Open(inStream, filePath, arcFormat, NULL, openArchiveCallback));
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
  // Exts[0].Ext;
  return StringToBstr(_codecs.Formats[formatIndex].GetAllExtensions(), extensions);
}
*/

static void AddIconExt(const CCodecIcons &lib, UString &dest)
{
  FOR_VECTOR (i, lib.IconPairs)
  {
    if (!dest.IsEmpty())
      dest += L' ';
    dest += lib.IconPairs[i].Ext;
  }
}

STDMETHODIMP CArchiveFolderManager::GetExtensions(BSTR *extensions)
{
  LoadFormats();
  *extensions = 0;
  UString res;
  FOR_VECTOR (i, _codecs->Libs)
    AddIconExt(_codecs->Libs[i], res);
  AddIconExt(_codecs->InternalIcons, res);
  return StringToBstr(res, extensions);
}

STDMETHODIMP CArchiveFolderManager::GetIconPath(const wchar_t *ext, BSTR *iconPath, Int32 *iconIndex)
{
  LoadFormats();
  *iconPath = 0;
  *iconIndex = 0;
  FOR_VECTOR (i, _codecs->Libs)
  {
    const CCodecLib &lib = _codecs->Libs[i];
    int ii;
    if (lib.FindIconIndex(ext, ii))
    {
      *iconIndex = ii;
      return StringToBstr(fs2us(lib.Path), iconPath);
    }
  }
  int ii;
  if (_codecs->InternalIcons.FindIconIndex(ext, ii))
  {
    FString path;
    if (NWindows::NDLL::MyGetModuleFileName(path))
    {
      *iconIndex = ii;
      return StringToBstr(fs2us(path), iconPath);
    }
  }
  return S_OK;
}

/*
STDMETHODIMP CArchiveFolderManager::GetTypes(BSTR *types)
{
  LoadFormats();
  UString typesStrings;
  FOR_VECTOR(i, _codecs.Formats)
  {
    const CArcInfoEx &ai = _codecs.Formats[i];
    if (ai.AssociateExts.Size() == 0)
      continue;
    if (i != 0)
      typesStrings += L' ';
    typesStrings += ai.Name;
  }
  return StringToBstr(typesStrings, types);
}
STDMETHODIMP CArchiveFolderManager::CreateFolderFile(const wchar_t * type,
    const wchar_t * filePath, IProgress progress)
{
  return E_NOTIMPL;
}
*/
