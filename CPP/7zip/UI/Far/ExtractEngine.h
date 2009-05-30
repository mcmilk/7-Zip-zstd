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
  STDMETHOD(SetTotal)(UInt64 size);
  STDMETHOD(SetCompleted)(const UInt64 *completeValue);

  // IExtractCallBack
  STDMETHOD(AskOverwrite)(
      const wchar_t *existName, const FILETIME *existTime, const UInt64 *existSize,
      const wchar_t *newName, const FILETIME *newTime, const UInt64 *newSize,
      Int32 *result);
  STDMETHOD (PrepareOperation)(const wchar_t *name, bool isFolder, Int32 askExtractMode, const UInt64 *position);

  STDMETHOD(MessageError)(const wchar_t *message);
  STDMETHOD(SetOperationResult)(Int32 resultEOperationResult, bool encrypted);
  // ICryptoGetTextPassword
  STDMETHOD(CryptoGetTextPassword)(BSTR *password);

private:
  UInt64 _total;
  UInt64 _processed;
  
  bool _totalIsDefined;
  bool _processedIsDefined;

  UString m_CurrentFilePath;

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
  CExtractCallBackImp(): _totalIsDefined(false), _processedIsDefined(false) {}
  ~CExtractCallBackImp();
  void Init(UINT codePage,
      CProgressBox *progressBox,
      bool passwordIsDefined, const UString &password);
};

#endif
