// TempFiles.cpp

#include "StdAfx.h"

#include "TempFiles.h"

#include "Windows/FileDir.h"
#include "Windows/FileIO.h"

using namespace NWindows;
using namespace NFile;

void CFileVectorBundle::DisableDeleting(int index)
{
  m_FileNames.Delete(index);
}

bool CFileVectorBundle::Add(const UString &filePath, bool tryToOpen)
{
  if (tryToOpen)
  {
    NIO::COutFile file;
    if (!file.Open(filePath))
      return false;
  }
  m_FileNames.Add(filePath);
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


