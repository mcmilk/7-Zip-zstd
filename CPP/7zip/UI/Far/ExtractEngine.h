// ExtractEngine.h

#ifndef ZIP7_INC_EXTRACT_ENGINE_H
#define ZIP7_INC_EXTRACT_ENGINE_H

#include "../../../Common/MyCom.h"
#include "../../../Common/MyString.h"

#include "../../IPassword.h"
#include "../Agent/IFolderArchive.h"

#include "ProgressBox.h"

Z7_CLASS_IMP_COM_3(
  CExtractCallbackImp
  , IFolderArchiveExtractCallback
  , IFolderArchiveExtractCallback2
  , ICryptoGetTextPassword
)
  Z7_IFACE_COM7_IMP(IProgress)

  UString m_CurrentFilePath;

  CProgressBox *_percent;
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
  void Init(UINT codePage,
      CProgressBox *progressBox,
      bool passwordIsDefined, const UString &password);
};

#endif
