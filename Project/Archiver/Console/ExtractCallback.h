// ExtractCallback.h

#pragma once

#ifndef __EXTRACTCALLBACK_H
#define __EXTRACTCALLBACK_H

#include "../Common/IArchiveHandler2.h"
#include "Common/String.h"


#include "Interface/FileStreams.h"
#include "../Common/ZipSettings.h"
#include "../../Compress/Interface./CompressInterface.h"
#include "../Format/Common/FormatCryptoInterface.h"

class CExtractCallBackImp: 
  public IExtractCallback200,
  public ICryptoGetTextPassword,
  public CComObjectRoot
{
public:
BEGIN_COM_MAP(CExtractCallBackImp)
  COM_INTERFACE_ENTRY(IExtractCallback200)
  COM_INTERFACE_ENTRY(ICryptoGetTextPassword)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CExtractCallBackImp)

DECLARE_NO_REGISTRY()

  // IProgress
  STDMETHOD(SetTotal)(UINT64 aSize);
  STDMETHOD(SetCompleted)(const UINT64 *aCompleteValue);

  // IExtractCallback200
  STDMETHOD(Extract)(UINT32 anIndex, ISequentialOutStream **anOutStream, 
      INT32 anAskExtractMode);
  STDMETHOD(PrepareOperation)(INT32 anAskExtractMode);
  STDMETHOD(OperationResult)(INT32 aResultEOperationResult);

  // ICryptoGetTextPassword
  STDMETHOD(CryptoGetTextPassword)(BSTR *aPassword);

private:
  CComPtr<IArchiveHandler200> m_ArchiveHandler;
  CSysString m_DirectoryPath;
  NZipSettings::NExtraction::CInfo m_ExtractModeInfo;

  CSysString m_FilePath;

  CSysString m_DiskFilePath;

  bool m_ExtractMode;
  struct CProcessedFileInfo
  {
    FILETIME UTCLastWriteTime;
    bool IsDirectory;
    bool AttributesAreDefined;
    UINT32 Attributes;
  } m_ProcessedFileInfo;

  CComObjectNoLock<COutFileStream> *m_OutFileStreamSpec;
  CComPtr<ISequentialOutStream> m_OutFileStream;
  UStringVector m_RemovePathParts;
  UINT m_CodePage;

  UString m_ItemDefaultName;
  FILETIME m_UTCLastWriteTimeDefault;
  UINT32 m_AttributesDefault;

  bool m_PasswordIsDefined;
  UString m_Password;

  void CreateComplexDirectory(const UStringVector &aDirPathParts);
  bool IsEncrypted(UINT32 anIndex);
public:
  void Init(IArchiveHandler200 *anArchiveHandler, const CSysString &aDirectoryPath,
      const NZipSettings::NExtraction::CInfo &anExtractModeInfo, 
      const UStringVector &aRemovePathParts,
      UINT aCodePage, 
      const UString &anItemDefaultName,
      const FILETIME &anUTCLastWriteTimeDefault, UINT32 anAttributesDefault,
      bool aPasswordIsDefined, const UString &aPassword);

  UINT64 m_NumErrors;
};

#endif
