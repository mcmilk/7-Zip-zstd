// ExtractCallback.h

#pragma once

#ifndef __EXTRACTCALLBACK_H
#define __EXTRACTCALLBACK_H

#include "Common/String.h"
#include "../../Common/FileStreams.h"
#include "../../IPassword.h"
#include "../../Archive/IArchive.h"
#include "../Common/ZipRegistry.h"

class CExtractCallbackImp: 
  public IArchiveExtractCallback,
  public ICryptoGetTextPassword,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP1(ICryptoGetTextPassword)

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
  CMyComPtr<IInArchive> m_ArchiveHandler;
  CSysString m_DirectoryPath;
  NExtraction::CInfo m_ExtractModeInfo;

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

  COutFileStream *m_OutFileStreamSpec;
  CMyComPtr<ISequentialOutStream> m_OutFileStream;
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
      const NExtraction::CInfo &anExtractModeInfo, 
      const UStringVector &removePathParts,
      UINT codePage, 
      const UString &itemDefaultName,
      const FILETIME &utcLastWriteTimeDefault, UINT32 attributesDefault,
      bool passwordIsDefined, const UString &password);

  UINT64 m_NumErrors;
};

#endif
