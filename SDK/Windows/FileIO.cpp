// Windows/FileIO.cpp

#include "StdAfx.h"

#include "Windows/FileIO.h"
#include "Windows/Defs.h"

#include "Common/Defs.h"
#include "Common/Types.h"

namespace NWindows {
namespace NFile {
namespace NIO {

CFileBase::~CFileBase()
{
  Close();
}

bool CFileBase::Create(LPCTSTR aFileName, DWORD aDesiredAccess,
    DWORD aShareMode, DWORD aCreationDisposition, DWORD aFlagsAndAttributes)
{
  Close();
  m_Handle = ::CreateFile(aFileName, aDesiredAccess, aShareMode, 
      (LPSECURITY_ATTRIBUTES)NULL, aCreationDisposition, 
      aFlagsAndAttributes, (HANDLE) NULL);
  m_FileIsOpen = m_Handle != INVALID_HANDLE_VALUE;
  return m_FileIsOpen;
}

bool CFileBase::Close()
{
  if(!m_FileIsOpen)
    return true;
  bool aResult = BOOLToBool(::CloseHandle(m_Handle));
  m_FileIsOpen = !aResult;
  return aResult;
}

bool CFileBase::GetPosition(UINT64 &aPosition) const
{
  return Seek(0, FILE_CURRENT, aPosition);
}

bool CFileBase::GetLength(UINT64 &aLength) const
{
  DWORD aSizeHigh;
  DWORD aSizeLow = ::GetFileSize(m_Handle, &aSizeHigh);
  if(aSizeLow == 0xFFFFFFFF)
    if(::GetLastError() != NO_ERROR )
      return false;
  aLength = (((UINT64)aSizeHigh) << 32) + aSizeLow;
  return true;
}

bool CFileBase::Seek(INT64 aDistanceToMove, 
    DWORD aMoveMethod, UINT64 &aNewFilePosition) const
{
  LARGE_INTEGER *aPointer = (LARGE_INTEGER *)&aDistanceToMove;
  aPointer->LowPart = ::SetFilePointer(m_Handle, aPointer->LowPart, 
      &aPointer->HighPart, aMoveMethod);
  if (aPointer->LowPart == 0xFFFFFFFF)
    if(::GetLastError() != NO_ERROR) 
      return false;
  aNewFilePosition = *((UINT64 *)aPointer);
  return true;
}

bool CFileBase::Seek(UINT64 aPosition, UINT64 &aNewPosition)
{
  return Seek(aPosition, FILE_BEGIN, aNewPosition);
}

bool CFileBase::SeekToBegin()
{
  UINT64 aNewPosition;
  return Seek(0, aNewPosition);
}

bool CFileBase::SeekToEnd(UINT64 &aNewPosition)
{
  return Seek(0, FILE_END, aNewPosition);
}

bool CFileBase::GetFileInformation(CByHandleFileInfo &aFileInfo) const
{
  BY_HANDLE_FILE_INFORMATION aWinFileInfo;
  if(!::GetFileInformationByHandle(m_Handle, &aWinFileInfo))
    return false;
  aFileInfo.Attributes = aWinFileInfo.dwFileAttributes;
  aFileInfo.CreationTime = aWinFileInfo.ftCreationTime;
  aFileInfo.LastAccessTime = aWinFileInfo.ftLastAccessTime;
  aFileInfo.LastWriteTime = aWinFileInfo.ftLastWriteTime;
  aFileInfo.VolumeSerialNumber = aWinFileInfo.dwFileAttributes; 
  aFileInfo.Size = (((UINT64)aWinFileInfo.nFileSizeHigh) << 32) + 
      aWinFileInfo.nFileSizeLow;
  aFileInfo.NumberOfLinks = aWinFileInfo.nNumberOfLinks;
  aFileInfo.FileIndex = (((UINT64)aWinFileInfo.nFileIndexHigh) << 32) + 
      aWinFileInfo.nFileIndexLow;
  return true;
}

/////////////////////////
// CInFile

bool CInFile::Open(LPCTSTR aFileName, DWORD aShareMode, 
    DWORD aCreationDisposition,  DWORD aFlagsAndAttributes)
{
  return Create(aFileName, GENERIC_READ, aShareMode, 
      aCreationDisposition, aFlagsAndAttributes);
}

bool CInFile::Open(LPCTSTR aFileName)
{
  return Open(aFileName, FILE_SHARE_READ, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL);
}

bool CInFile::Read(void *aData, UINT32 aSize, UINT32 &aProcessedSize)
{
  return BOOLToBool(::ReadFile(m_Handle, aData, aSize, 
      (DWORD *)&aProcessedSize, NULL));
}

/////////////////////////
// COutFile

bool COutFile::Open(LPCTSTR aFileName, DWORD aShareMode, 
    DWORD aCreationDisposition, DWORD aFlagsAndAttributes)
{
  return Create(aFileName, GENERIC_WRITE, aShareMode, 
      aCreationDisposition, aFlagsAndAttributes);
}

bool COutFile::Open(LPCTSTR aFileName)
{
  return Open(aFileName, FILE_SHARE_READ, m_CreationDisposition, FILE_ATTRIBUTE_NORMAL);
}

bool COutFile::SetTime(const FILETIME *aCreationTime,
  const FILETIME *aLastAccessTime, const FILETIME *aLastWriteTime)
{
  return BOOLToBool(::SetFileTime(m_Handle, aCreationTime,
      aLastAccessTime, aLastWriteTime));
}

bool COutFile::SetLastWriteTime(const FILETIME *aLastWriteTime)
{
  return SetTime(NULL, NULL, aLastWriteTime);
}

bool COutFile::Write(const void *aData, UINT32 aSize, UINT32 &aProcessedSize)
{
  return BOOLToBool(::WriteFile(m_Handle, aData, aSize, 
      (DWORD *)&aProcessedSize, NULL));
}

bool COutFile::SetEndOfFile()
{
  return BOOLToBool(::SetEndOfFile(m_Handle));
}

bool COutFile::SetLength(UINT64 aLength)
{
  UINT64 aNewPosition;
  if(!Seek(aLength, aNewPosition))
    return false;
  if(aNewPosition != aLength)
    return false;
  return SetEndOfFile();
}

}}}
