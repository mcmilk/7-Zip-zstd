// Windows/FileIO.h

#pragma once

#ifndef __WINDOWS_FILEIO_H
#define __WINDOWS_FILEIO_H

namespace NWindows {
namespace NFile {
namespace NIO {

struct CByHandleFileInfo
{ 
  DWORD    Attributes; 
  FILETIME CreationTime; 
  FILETIME LastAccessTime; 
  FILETIME LastWriteTime; 
  DWORD    VolumeSerialNumber; 
  UINT64   Size;
  DWORD    NumberOfLinks; 
  UINT64   FileIndex; 
};

class CFileBase
{
protected:
  bool m_FileIsOpen;
  HANDLE m_Handle;
  bool Create(LPCTSTR aFileName, DWORD aDesiredAccess,
      DWORD aShareMode, DWORD aCreationDisposition,  DWORD aFlagsAndAttributes);
public:
  CFileBase():
    m_FileIsOpen(false){};
  virtual ~CFileBase();

  virtual bool Close();

  bool GetPosition(UINT64 &aPosition) const;
  bool GetLength(UINT64 &aLength) const;

  bool Seek(INT64 aDistanceToMove, 
      DWORD aMoveMethod, UINT64 &aNewFilePosition) const;
  bool Seek(UINT64 aPosition, UINT64 &aNewPosition); 
  bool SeekToBegin(); 
  bool SeekToEnd(UINT64 &aNewPosition); 
  
  bool GetFileInformation(CByHandleFileInfo &aFileInfo) const;
};

class CInFile: public CFileBase
{
public:
  bool Open(LPCTSTR aFileName,
      DWORD aShareMode, DWORD aCreationDisposition,  DWORD aFlagsAndAttributes);
  bool Open(LPCTSTR aFileName);
  bool Read(void *aData, UINT32 aSize, UINT32 &aProcessedSize);
};

class COutFile: public CFileBase
{
  DWORD m_CreationDisposition;
public:
  COutFile(): m_CreationDisposition(CREATE_NEW){};
  bool Open(LPCTSTR aFileName, DWORD aShareMode, 
      DWORD aCreationDisposition, DWORD aFlagsAndAttributes);
  bool Open(LPCTSTR aFileName);

  void SetOpenCreationDisposition(DWORD aCreationDisposition)
    { m_CreationDisposition = aCreationDisposition; }
  void SetOpenCreationDispositionCreateAlways()
    { m_CreationDisposition = CREATE_ALWAYS; }

  bool SetTime(const FILETIME *aCreationTime,
      const FILETIME *aLastAccessTime, const FILETIME *aLastWriteTime);
  bool SetLastWriteTime(const FILETIME *aLastWriteTime);
  bool Write(const void *aData, UINT32 aSize, UINT32 &aProcessedSize);
  bool SetEndOfFile();
  bool SetLength(UINT64 aLength);
};

}}}

#endif
