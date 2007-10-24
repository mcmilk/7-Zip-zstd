// FileStreams.h

#ifndef __FILESTREAMS_H
#define __FILESTREAMS_H

#ifdef _WIN32
#define USE_WIN_FILE
#endif

#ifdef USE_WIN_FILE
#include "../../Windows/FileIO.h"
#else
#include "../../Common/C_FileIO.h"
#endif

#include "../IStream.h"
#include "../../Common/MyCom.h"

class CInFileStream: 
  public IInStream,
  public IStreamGetSize,
  public CMyUnknownImp
{
public:
  #ifdef USE_WIN_FILE
  NWindows::NFile::NIO::CInFile File;
  #else
  NC::NFile::NIO::CInFile File;
  #endif
  CInFileStream() {}
  virtual ~CInFileStream() {}

  bool Open(LPCTSTR fileName);
  #ifdef USE_WIN_FILE
  #ifndef _UNICODE
  bool Open(LPCWSTR fileName);
  #endif
  #endif

  bool OpenShared(LPCTSTR fileName, bool shareForWrite);
  #ifdef USE_WIN_FILE
  #ifndef _UNICODE
  bool OpenShared(LPCWSTR fileName, bool shareForWrite);
  #endif
  #endif

  MY_UNKNOWN_IMP2(IInStream, IStreamGetSize)

  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);
  STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition);

  STDMETHOD(GetSize)(UInt64 *size);
};

#ifndef _WIN32_WCE
class CStdInFileStream: 
  public ISequentialInStream,
  public CMyUnknownImp
{
public:
  // HANDLE File;
  // CStdInFileStream() File(INVALID_HANDLE_VALUE): {}
  // void Open() { File = GetStdHandle(STD_INPUT_HANDLE); };
  MY_UNKNOWN_IMP

  virtual ~CStdInFileStream() {}
  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);
};
#endif

class COutFileStream: 
  public IOutStream,
  public CMyUnknownImp
{
  #ifdef USE_WIN_FILE
  NWindows::NFile::NIO::COutFile File;
  #else
  NC::NFile::NIO::COutFile File;
  #endif
public:
  virtual ~COutFileStream() {}
  bool Create(LPCTSTR fileName, bool createAlways)
  {
    ProcessedSize = 0;
    return File.Create(fileName, createAlways);
  }
  bool Open(LPCTSTR fileName, DWORD creationDisposition)
  {
    ProcessedSize = 0;
    return File.Open(fileName, creationDisposition);
  }
  #ifdef USE_WIN_FILE
  #ifndef _UNICODE
  bool Create(LPCWSTR fileName, bool createAlways)
  {
    ProcessedSize = 0;
    return File.Create(fileName, createAlways);
  }
  bool Open(LPCWSTR fileName, DWORD creationDisposition)
  {
    ProcessedSize = 0;
    return File.Open(fileName, creationDisposition);
  }
  #endif
  #endif

  HRESULT Close();
  
  UInt64 ProcessedSize;

  #ifdef USE_WIN_FILE
  bool SetTime(const FILETIME *creationTime, const FILETIME *lastAccessTime, const FILETIME *lastWriteTime)
  {
    return File.SetTime(creationTime, lastAccessTime, lastWriteTime);
  }
  bool SetLastWriteTime(const FILETIME *lastWriteTime)
  {
    return File.SetLastWriteTime(lastWriteTime);
  }
  #endif


  MY_UNKNOWN_IMP1(IOutStream)

  STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);
  STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition);
  STDMETHOD(SetSize)(Int64 newSize);
};

#ifndef _WIN32_WCE
class CStdOutFileStream: 
  public ISequentialOutStream,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP

  virtual ~CStdOutFileStream() {}
  STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);
};
#endif

#endif
