// FileCreationUtils.cpp

#include "StdAfx.h"

#include "FileCreationUtils.h"

#include "Windows/FileDir.h"
#include "Windows/FileIO.h"

using namespace NWindows;
using namespace NFile;

void CFileVectorBundle::DisableDeleting(int anIndex)
{
  m_FileNames.Delete(anIndex);
}

bool CFileVectorBundle::Add(const CSysString &aFilePath, bool aTryToOpen)
{
  if (aTryToOpen)
  {
    NIO::COutFile aFile;
    if (!aFile.Open(aFilePath))
      return false;
  }
  m_FileNames.Add(aFilePath);
  return true;
}

void CFileVectorBundle::Clear()
{
  while(!m_FileNames.IsEmpty())
  {
    NDirectory::DeleteFileAlways(m_FileNames.Back());
    m_FileNames.DeleteBack();
  }
}

CFileVectorBundle::~CFileVectorBundle()
{
  Clear();
}


