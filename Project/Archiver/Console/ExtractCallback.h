// ExtractCallback.h

#pragma once

#ifndef __EXTRACTCALLBACK_H
#define __EXTRACTCALLBACK_H

#include "Common/String.h"
#include "Interface/FileStreams.h"
#include "Interface/CryptoInterface.h"

#include "../Format/Common/ArchiveInterface.h"
#include "../Common/ZipSettings.h"

class CExtractCallbackImp: 
  public IArchiveExtractCallback,
  public ICryptoGetTextPassword,
  public CComObjectRoot
{
public:
BEGIN_COM_MAP(CExtractCallbackImp)
  COM_INTERFACE_ENTRY(IArchiveExtractCallback)
  COM_INTERFACE_ENTRY(ICryptoGetTextPassword)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CExtractCallbackImp)

DECLARE_NO_REGISTRY()

  // IProgress
  STDMETHOD(SetTotal)(UINT64 size);
  STDMETHOD(SetCompleted)(const UINT64 *completeValue);

  // IExtractCallback200
  STDMETHOD(GetStream)(UINT32 index, ISequentialOutStream **outStream, 
      INT32 askExtractMode);
  STDMETHOD(PrepareOperation)(INT32 askExtractMode);
  STDMETHOD(SetOperationResult)(INT32 resultEOperationResult);

  // ICryptoGetTextPassword
  STDMETHOD(CryptoGetTextPassword)(BSTR *password);

private:
  CComPtr<IInArchive> m_ArchiveHandler;
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

  void CreateComplexDirectory(const UStringVector &dirPathParts);
  bool IsEncrypted(UINT32 index);
public:
  void Init(IInArchive *archive, const CSysString &directoryPath,
      const NZipSettings::NExtraction::CInfo &anExtractModeInfo, 
      const UStringVector &removePathParts,
      UINT codePage, 
      const UString &itemDefaultName,
      const FILETIME &utcLastWriteTimeDefault, UINT32 attributesDefault,
      bool passwordIsDefined, const UString &password);

  UINT64 m_NumErrors;
};

#endif
