// ExtractIcon.cpp

#include "StdAfx.h"

#include "ExtractIcon.h"
#include "Common/StringConvert.h"
#include "Windows/PropVariant.h"
#include "Windows/Defs.h"

#include "MyIDList.h"

using namespace NWindows;
using namespace NCOM;

void CExtractIconImp::Init(IArchiveFolder *anArchiveFolder, UINT32 anIndex)
{
  m_ArchiveFolder = anArchiveFolder;
  m_Index = anIndex;
}

static const TCHAR kZipViewID[] = _T("ZipView::"); 
static const kZipViewIDSize = sizeof(kZipViewID) / sizeof(kZipViewID[0]) - 1; 

enum
{
  kFile = 0,
  kClosed,
  kOpen
};

STDMETHODIMP CExtractIconImp::GetIconLocation(
    UINT uFlags,
    LPTSTR szIconFile,
    UINT cchMax,
    int *piIndex,
    UINT *pwFlags)
{
  *pwFlags = GIL_PERCLASS | GIL_NOTFILENAME;

  CSysString aSysString = kZipViewID;
  aSysString += GetSystemString(GetNameOfObject(m_ArchiveFolder, m_Index));
  if(IsObjectFolder(m_ArchiveFolder, m_Index))
    *piIndex = (uFlags & GIL_OPENICON) ? kOpen : kClosed; 
  else
    *piIndex = kFile; 
  if(cchMax + 1 < UINT(aSysString.Length()))
    return E_INVALIDARG;
  lstrcpy(szIconFile, aSysString);
  return NOERROR;
}

STDMETHODIMP CExtractIconImp::Extract(
    LPCTSTR pszFile,
    UINT nIconIndex,
    HICON *phiconLarge,
    HICON *phiconSmall,
    UINT nIconSize)
{
  CSysString aFullName = pszFile;
  CSysString anIDString = aFullName.Left(kZipViewIDSize);
  if(anIDString != kZipViewID)
    return(E_INVALIDARG);
  
  UINT aFlags = SHGFI_ICON | SHGFI_USEFILEATTRIBUTES; 
  CSysString aSysString = aFullName.Mid(kZipViewIDSize);
  DWORD anAttributes = FILE_ATTRIBUTE_NORMAL;
  if(nIconIndex != kFile)
  {
    if(nIconIndex == kOpen)
      aFlags |= SHGFI_OPENICON; 
    else if(nIconIndex != kClosed)
      return(E_INVALIDARG); 
    anAttributes |= FILE_ATTRIBUTE_DIRECTORY;
  }

  SHFILEINFO aFileInfo;

  *phiconLarge = *phiconSmall = NULL; 

  if (!SHGetFileInfo(aSysString, anAttributes, &aFileInfo, sizeof(aFileInfo), 
      aFlags | SHGFI_LARGEICON)) 
    return(E_UNEXPECTED); 
  
  *phiconLarge = aFileInfo.hIcon; 
  
  if (SHGetFileInfo(aSysString, anAttributes, &aFileInfo, sizeof(aFileInfo), 
      aFlags | SHGFI_SMALLICON)) 
    *phiconSmall = aFileInfo.hIcon; 
  
  return(S_OK); 
}
