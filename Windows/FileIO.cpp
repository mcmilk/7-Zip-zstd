// Windows/FileIO.cpp

#include "StdAfx.h"

#include "FileIO.h"
#include "Defs.h"
#ifndef _UNICODE
#include "../Common/StringConvert.h"
#endif

namespace NWindows {
namespace NFile {
namespace NIO {

CFileBase::~CFileBase()
{
  Close();
}

bool CFileBase::Create(LPCTSTR fileName, DWORD desiredAccess,
    DWORD shareMode, DWORD creationDisposition, DWORD flagsAndAttributes)
{
  Close();
  _handle = ::CreateFile(fileName, desiredAccess, shareMode, 
      (LPSECURITY_ATTRIBUTES)NULL, creationDisposition, 
      flagsAndAttributes, (HANDLE) NULL);
  _fileIsOpen = _handle != INVALID_HANDLE_VALUE;
  return _fileIsOpen;
}

#ifndef _UNICODE
bool CFileBase::Create(LPCWSTR fileName, DWORD desiredAccess,
    DWORD shareMode, DWORD creationDisposition, DWORD flagsAndAttributes)
{
  Close();
  // MessageBoxW(0, fileName, 0, 0);
  // ::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  _handle = ::CreateFileW(fileName, desiredAccess, shareMode, 
      (LPSECURITY_ATTRIBUTES)NULL, creationDisposition, 
      flagsAndAttributes, (HANDLE) NULL);
  if ((_handle == INVALID_HANDLE_VALUE ||  _handle == 0) &&
      ::GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    return Create(UnicodeStringToMultiByte(fileName, ::AreFileApisANSI() ? CP_ACP : CP_OEMCP), 
      desiredAccess, shareMode, creationDisposition, flagsAndAttributes);
  return (_fileIsOpen = _handle != INVALID_HANDLE_VALUE);
}
#endif

bool CFileBase::Close()
{
  if(!_fileIsOpen)
    return true;
  bool result = BOOLToBool(::CloseHandle(_handle));
  _fileIsOpen = !result;
  return result;
}

bool CFileBase::GetPosition(UINT64 &position) const
{
  return Seek(0, FILE_CURRENT, position);
}

bool CFileBase::GetLength(UINT64 &length) const
{
  DWORD sizeHigh;
  DWORD sizeLow = ::GetFileSize(_handle, &sizeHigh);
  if(sizeLow == 0xFFFFFFFF)
    if(::GetLastError() != NO_ERROR )
      return false;
  length = (((UINT64)sizeHigh) << 32) + sizeLow;
  return true;
}

bool CFileBase::Seek(INT64 distanceToMove, DWORD moveMethod, UINT64 &newPosition) const
{
  LARGE_INTEGER *pointer = (LARGE_INTEGER *)&distanceToMove;
  pointer->LowPart = ::SetFilePointer(_handle, pointer->LowPart, 
      &pointer->HighPart, moveMethod);
  if (pointer->LowPart == 0xFFFFFFFF)
    if(::GetLastError() != NO_ERROR) 
      return false;
  newPosition = *((UINT64 *)pointer);
  return true;
}

bool CFileBase::Seek(UINT64 position, UINT64 &newPosition)
{
  return Seek(position, FILE_BEGIN, newPosition);
}

bool CFileBase::SeekToBegin()
{
  UINT64 newPosition;
  return Seek(0, newPosition);
}

bool CFileBase::SeekToEnd(UINT64 &newPosition)
{
  return Seek(0, FILE_END, newPosition);
}

bool CFileBase::GetFileInformation(CByHandleFileInfo &fileInfo) const
{
  BY_HANDLE_FILE_INFORMATION winFileInfo;
  if(!::GetFileInformationByHandle(_handle, &winFileInfo))
    return false;
  fileInfo.Attributes = winFileInfo.dwFileAttributes;
  fileInfo.CreationTime = winFileInfo.ftCreationTime;
  fileInfo.LastAccessTime = winFileInfo.ftLastAccessTime;
  fileInfo.LastWriteTime = winFileInfo.ftLastWriteTime;
  fileInfo.VolumeSerialNumber = winFileInfo.dwFileAttributes; 
  fileInfo.Size = (((UINT64)winFileInfo.nFileSizeHigh) << 32) + 
      winFileInfo.nFileSizeLow;
  fileInfo.NumberOfLinks = winFileInfo.nNumberOfLinks;
  fileInfo.FileIndex = (((UINT64)winFileInfo.nFileIndexHigh) << 32) + 
      winFileInfo.nFileIndexLow;
  return true;
}

/////////////////////////
// CInFile

bool CInFile::Open(LPCTSTR fileName, DWORD shareMode, 
    DWORD creationDisposition,  DWORD flagsAndAttributes)
{
  return Create(fileName, GENERIC_READ, shareMode, 
      creationDisposition, flagsAndAttributes);
}

bool CInFile::Open(LPCTSTR fileName)
{
  return Open(fileName, FILE_SHARE_READ, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL);
}

#ifndef _UNICODE
bool CInFile::Open(LPCWSTR fileName, DWORD shareMode, 
    DWORD creationDisposition,  DWORD flagsAndAttributes)
{
  return Create(fileName, GENERIC_READ, shareMode, 
      creationDisposition, flagsAndAttributes);
}

bool CInFile::Open(LPCWSTR fileName)
{
  return Open(fileName, FILE_SHARE_READ, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL);
}
#endif

bool CInFile::Read(void *data, UINT32 size, UINT32 &processedSize)
{
  return BOOLToBool(::ReadFile(_handle, data, size, 
      (DWORD *)&processedSize, NULL));
}

/////////////////////////
// COutFile

bool COutFile::Open(LPCTSTR fileName, DWORD shareMode, 
    DWORD creationDisposition, DWORD flagsAndAttributes)
{
  return Create(fileName, GENERIC_WRITE, shareMode, 
      creationDisposition, flagsAndAttributes);
}

bool COutFile::Open(LPCTSTR fileName)
{
  return Open(fileName, FILE_SHARE_READ, m_CreationDisposition, FILE_ATTRIBUTE_NORMAL);
}

#ifndef _UNICODE

bool COutFile::Open(LPCWSTR fileName, DWORD shareMode, 
    DWORD creationDisposition, DWORD flagsAndAttributes)
{
  return Create(fileName, GENERIC_WRITE, shareMode, 
      creationDisposition, flagsAndAttributes);
}

bool COutFile::Open(LPCWSTR fileName)
{
  return Open(fileName, FILE_SHARE_READ, m_CreationDisposition, FILE_ATTRIBUTE_NORMAL);
}

#endif

bool COutFile::SetTime(const FILETIME *creationTime,
  const FILETIME *lastAccessTime, const FILETIME *lastWriteTime)
{
  return BOOLToBool(::SetFileTime(_handle, creationTime,
      lastAccessTime, lastWriteTime));
}

bool COutFile::SetLastWriteTime(const FILETIME *lastWriteTime)
{
  return SetTime(NULL, NULL, lastWriteTime);
}

bool COutFile::Write(const void *data, UINT32 size, UINT32 &processedSize)
{
  return BOOLToBool(::WriteFile(_handle, data, size, 
      (DWORD *)&processedSize, NULL));
}

bool COutFile::SetEndOfFile()
{
  return BOOLToBool(::SetEndOfFile(_handle));
}

bool COutFile::SetLength(UINT64 length)
{
  UINT64 newPosition;
  if(!Seek(length, newPosition))
    return false;
  if(newPosition != length)
    return false;
  return SetEndOfFile();
}

}}}
