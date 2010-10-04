// AppState.h

#ifndef __APP_STATE_H
#define __APP_STATE_H

#include "Windows/Synchronization.h"

#include "ViewSettings.h"

void inline AddUniqueStringToHead(UStringVector &list,
    const UString &string)
{
  for(int i = 0; i < list.Size();)
    if (string.CompareNoCase(list[i]) == 0)
      list.Delete(i);
    else
      i++;
  list.Insert(0, string);
}

class CFastFolders
{
  NWindows::NSynchronization::CCriticalSection _criticalSection;
public:
  UStringVector Strings;
  void SetString(int index, const UString &string)
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_criticalSection);
    while(Strings.Size() <= index)
      Strings.Add(UString());
    Strings[index] = string;
  }
  UString GetString(int index)
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_criticalSection);
    if (index >= Strings.Size())
      return UString();
    return Strings[index];
  }
  void Save()
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_criticalSection);
    SaveFastFolders(Strings);
  }
  void Read()
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_criticalSection);
    ReadFastFolders(Strings);
  }
};

class CFolderHistory
{
  NWindows::NSynchronization::CCriticalSection _criticalSection;
  UStringVector Strings;
  void Normalize()
  {
    const int kMaxSize = 100;
    if (Strings.Size() > kMaxSize)
      Strings.Delete(kMaxSize, Strings.Size() - kMaxSize);
  }
  
public:
  
  void GetList(UStringVector &foldersHistory)
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_criticalSection);
    foldersHistory = Strings;
  }
  
  void AddString(const UString &string)
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_criticalSection);
    AddUniqueStringToHead(Strings, string);
    Normalize();
  }
  
  void RemoveAll()
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_criticalSection);
    Strings.Clear();
  }
  
  void Save()
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_criticalSection);
    SaveFolderHistory(Strings);
  }
  
  void Read()
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_criticalSection);
    ReadFolderHistory(Strings);
    Normalize();
  }
};

struct CAppState
{
  CFastFolders FastFolders;
  CFolderHistory FolderHistory;
  void Save()
  {
    FastFolders.Save();
    FolderHistory.Save();
  }
  void Read()
  {
    FastFolders.Read();
    FolderHistory.Read();
  }
};

#endif
