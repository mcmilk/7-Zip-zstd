// Windows/FileFind.h

#pragma once

#ifndef __WINDOWS_FILEFIND_H
#define __WINDOWS_FILEFIND_H

#include "Common/String.h"
#include "Windows/FileName.h"
#include "Windows/Defs.h"

namespace NWindows {
namespace NFile {
namespace NFind {

namespace NAttributes
{
  inline bool IsReadOnly(DWORD anAttributes) { return (anAttributes & FILE_ATTRIBUTE_READONLY) != 0; }
  inline bool IsHidden(DWORD anAttributes) { return (anAttributes & FILE_ATTRIBUTE_HIDDEN) != 0; }
  inline bool IsSystem(DWORD anAttributes) { return (anAttributes & FILE_ATTRIBUTE_SYSTEM) != 0; }
  inline bool IsDirectory(DWORD anAttributes) { return (anAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0; }
  inline bool IsArchived(DWORD anAttributes) { return (anAttributes & FILE_ATTRIBUTE_ARCHIVE) != 0; }
  inline bool IsCompressed(DWORD anAttributes) { return (anAttributes & FILE_ATTRIBUTE_COMPRESSED) != 0; }
  inline bool IsEncrypted(DWORD anAttributes) { return (anAttributes & FILE_ATTRIBUTE_ENCRYPTED) != 0; }
}

class CFileInfo
{ 
  bool MatchesMask(UINT32 aMask) const  { return ((Attributes & aMask) != 0); }
public:
  DWORD Attributes;
  FILETIME CreationTime;  
  FILETIME LastAccessTime; 
  FILETIME LastWriteTime;
  UINT64 Size;
  
  #ifndef _WIN32_WCE
  UINT32 ReparseTag;
  #else
  DWORD ObjectID; 
  #endif


  CSysString Name;
  bool IsArchived() const { return MatchesMask(FILE_ATTRIBUTE_ARCHIVE); }
  bool IsCompressed() const { return MatchesMask(FILE_ATTRIBUTE_COMPRESSED); }
  bool IsDirectory() const { return MatchesMask(FILE_ATTRIBUTE_DIRECTORY); }
  bool IsEncrypted() const { return MatchesMask(FILE_ATTRIBUTE_ENCRYPTED); }
  bool IsHidden() const { return MatchesMask(FILE_ATTRIBUTE_HIDDEN); }
  bool IsNormal() const { return MatchesMask(FILE_ATTRIBUTE_NORMAL); }
  bool IsOffline() const { return MatchesMask(FILE_ATTRIBUTE_OFFLINE); }
  bool IsReadOnly() const { return MatchesMask(FILE_ATTRIBUTE_READONLY); }
  bool HasReparsePoint() const { return MatchesMask(FILE_ATTRIBUTE_REPARSE_POINT); }
  bool IsSparse() const { return MatchesMask(FILE_ATTRIBUTE_SPARSE_FILE); }
  bool IsSystem() const { return MatchesMask(FILE_ATTRIBUTE_SYSTEM); }
  bool IsTemporary() const { return MatchesMask(FILE_ATTRIBUTE_TEMPORARY); }

  bool IsDots() const;
};

class CFindFile
{
  friend class CEnumerator;
  HANDLE m_Handle;
  bool m_HandleAllocated;
protected:
  bool IsHandleAllocated() const { return m_HandleAllocated; }
public:
  CFindFile(): m_HandleAllocated(false) {}
  ~CFindFile() {  Close(); }
  bool FindFirst(LPCTSTR aWildcard, CFileInfo &aFileInfo);
  bool FindNext(CFileInfo &aFileInfo);
  bool Close();
};

bool FindFile(LPCTSTR aWildcard, CFileInfo &aFileInfo);
bool DoesFileExist(LPCTSTR aName);

class CEnumerator
{
  CFindFile m_FindFile;
  CSysString m_Wildcard;
  bool NextAny(CFileInfo &aFileInfo);
public:
  CEnumerator(): m_Wildcard(NName::kAnyStringWildcard) {}
  CEnumerator(const CSysString &aWildcard): m_Wildcard(aWildcard) {}
  bool Next(CFileInfo &aFileInfo);
};

class CFindChangeNotification
{
  HANDLE m_Handle;
public:
  operator HANDLE () { return m_Handle; }
  CFindChangeNotification(): m_Handle(INVALID_HANDLE_VALUE) {}
  ~CFindChangeNotification() {  Close(); }
  bool Close();
  HANDLE FindFirst(LPCTSTR aPathName, bool aWatchSubtree, DWORD aNotifyFilter);
  bool FindNext()
    { return BOOLToBool(::FindNextChangeNotification(m_Handle)); }
};

#ifndef _WIN32_WCE
bool MyGetLogicalDriveStrings(CSysStringVector &aDriveStrings);
#endif

}}}

#endif

