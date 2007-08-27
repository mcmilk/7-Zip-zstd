// ExtractEngine.h

#ifndef __EXTRACTENGINE_H
#define __EXTRACTENGINE_H

#include "Common/MyCom.h"
#include "Common/MyString.h"

#include "../../IPassword.h"
#include "../Agent/IFolderArchive.h"

#include "ProgressBox.h"

class CExtractCallBackImp: 
  public IFolderArchiveExtractCallback,
  public ICryptoGetTextPassword,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP1(ICryptoGetTextPassword)

  // IProgress
  STDMETHOD(SetTotal)(UINT64 size);
  STDMETHOD(SetCompleted)(const UINT64 *completeValue);

  // IExtractCallBack
  STDMETHOD(AskOverwrite)(
      const wchar_t *existName, const FILETIME *existTime, const UINT64 *existSize,
      const wchar_t *newName, const FILETIME *newTime, const UINT64 *newSize,
      INT32 *result);
  STDMETHOD (PrepareOperation)(const wchar_t *name, bool isFolder, INT32 askExtractMode, const UINT64 *position);

  STDMETHOD(MessageError)(const wchar_t *message);
  STDMETHOD(SetOperationResult)(INT32 resultEOperationResult, bool encrypted);
  // ICryptoGetTextPassword
  STDMETHOD(CryptoGetTextPassword)(BSTR *password);

private:
  UString m_CurrentFilePath;

  struct CProcessedFileInfo
  {
    FILETIME UTCLastWriteTime;
    bool IsDirectory;
    UINT32 Attributes;
  } m_ProcessedFileInfo;

  CProgressBox *m_ProgressBox;
  UINT m_CodePage;

  bool m_PasswordIsDefined;
  UString m_Password;

  void CreateComplexDirectory(const UStringVector &dirPathParts);
  /*
  void GetPropertyValue(LPITEMIDLIST anItemIDList, PROPID aPropId, 
      PROPVARIANT *aValue);
  bool IsEncrypted(LPITEMIDLIST anItemIDList);
  */
  void AddErrorMessage(LPCTSTR message);
public:
  ~CExtractCallBackImp();
  void Init(UINT codePage, 
      CProgressBox *progressBox, 
      bool passwordIsDefined, const UString &password);
};

#endif
