// Windows/FileIO.h

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
  bool _fileIsOpen;
  HANDLE _handle;
  bool Create(LPCTSTR fileName, DWORD desiredAccess,
      DWORD shareMode, DWORD creationDisposition,  DWORD flagsAndAttributes);
  #ifndef _UNICODE
  bool Create(LPCWSTR fileName, DWORD desiredAccess,
      DWORD shareMode, DWORD creationDisposition,  DWORD flagsAndAttributes);
  #endif

public:
  CFileBase():
    _fileIsOpen(false){};
  virtual ~CFileBase();

  virtual bool Close();

  bool GetPosition(UINT64 &position) const;
  bool GetLength(UINT64 &length) const;

  bool Seek(INT64 distanceToMove, DWORD moveMethod, UINT64 &newPosition) const;
  bool Seek(UINT64 position, UINT64 &newPosition); 
  bool SeekToBegin(); 
  bool SeekToEnd(UINT64 &newPosition); 
  
  bool GetFileInformation(CByHandleFileInfo &fileInfo) const;
};

class CInFile: public CFileBase
{
public:
  bool Open(LPCTSTR fileName, DWORD shareMode, 
      DWORD creationDisposition,  DWORD flagsAndAttributes);
  bool Open(LPCTSTR fileName);
  #ifndef _UNICODE
  bool Open(LPCWSTR fileName, DWORD shareMode, 
      DWORD creationDisposition,  DWORD flagsAndAttributes);
  bool Open(LPCWSTR fileName);
  #endif
  bool Read(void *data, UINT32 size, UINT32 &processedSize);
};

class COutFile: public CFileBase
{
  // DWORD m_CreationDisposition;
public:
  // COutFile(): m_CreationDisposition(CREATE_NEW){};
  bool Open(LPCTSTR fileName, DWORD shareMode, 
      DWORD creationDisposition, DWORD flagsAndAttributes);
  bool Open(LPCTSTR fileName, DWORD creationDisposition);
  bool Create(LPCTSTR fileName, bool createAlways);

  #ifndef _UNICODE
  bool Open(LPCWSTR fileName, DWORD shareMode, 
      DWORD creationDisposition, DWORD flagsAndAttributes);
  bool Open(LPCWSTR fileName, DWORD creationDisposition);
  bool Create(LPCWSTR fileName, bool createAlways);
  #endif

  /*
  void SetOpenCreationDisposition(DWORD creationDisposition)
    { m_CreationDisposition = creationDisposition; }
  void SetOpenCreationDispositionCreateAlways()
    { m_CreationDisposition = CREATE_ALWAYS; }
  */

  bool SetTime(const FILETIME *creationTime,
      const FILETIME *lastAccessTime, const FILETIME *lastWriteTime);
  bool SetLastWriteTime(const FILETIME *lastWriteTime);
  bool Write(const void *data, UINT32 size, UINT32 &processedSize);
  bool SetEndOfFile();
  bool SetLength(UINT64 length);
};

}}}

#endif
