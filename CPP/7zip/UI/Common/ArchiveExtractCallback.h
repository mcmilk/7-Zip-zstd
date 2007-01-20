// ArchiveExtractCallback.h

#ifndef __ARCHIVEEXTRACTCALLBACK_H
#define __ARCHIVEEXTRACTCALLBACK_H

#include "../../Archive/IArchive.h"
#include "IFileExtractCallback.h"

#include "Common/String.h"
#include "Common/MyCom.h"

#include "../../Common/FileStreams.h"
#include "../../IPassword.h"

#include "ExtractMode.h"

class CArchiveExtractCallback: 
  public IArchiveExtractCallback,
  // public IArchiveVolumeExtractCallback,
  public ICryptoGetTextPassword,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP1(ICryptoGetTextPassword)
  // COM_INTERFACE_ENTRY(IArchiveVolumeExtractCallback)

  // IProgress
  STDMETHOD(SetTotal)(UInt64 size);
  STDMETHOD(SetCompleted)(const UInt64 *completeValue);

  // IExtractCallBack
  STDMETHOD(GetStream)(UInt32 index, ISequentialOutStream **outStream, Int32 askExtractMode);
  STDMETHOD(PrepareOperation)(Int32 askExtractMode);
  STDMETHOD(SetOperationResult)(Int32 resultEOperationResult);

  // IArchiveVolumeExtractCallback
  // STDMETHOD(GetInStream)(const wchar_t *name, ISequentialInStream **inStream);

  // ICryptoGetTextPassword
  STDMETHOD(CryptoGetTextPassword)(BSTR *aPassword);

private:
  CMyComPtr<IInArchive> _archiveHandler;
  CMyComPtr<IFolderArchiveExtractCallback> _extractCallback2;
  CMyComPtr<ICryptoGetTextPassword> _cryptoGetTextPassword;
  UString _directoryPath;
  NExtract::NPathMode::EEnum _pathMode;
  NExtract::NOverwriteMode::EEnum _overwriteMode;

  UString _filePath;
  UInt64 _position;
  bool _isSplit;

  UString _diskFilePath;

  bool _extractMode;

  bool WriteModified;
  bool WriteCreated;
  bool WriteAccessed;

  bool _encrypted;

  struct CProcessedFileInfo
  {
    FILETIME CreationTime;
    FILETIME LastWriteTime;
    FILETIME LastAccessTime;
    UInt32 Attributes;
  
    bool IsCreationTimeDefined;
    bool IsLastWriteTimeDefined;
    bool IsLastAccessTimeDefined;

    bool IsDirectory;
    bool AttributesAreDefined;
  } _processedFileInfo;

  COutFileStream *_outFileStreamSpec;
  CMyComPtr<ISequentialOutStream> _outFileStream;
  UStringVector _removePathParts;

  UString _itemDefaultName;
  FILETIME _utcLastWriteTimeDefault;
  UInt32 _attributesDefault;
  bool _stdOutMode;

  void CreateComplexDirectory(const UStringVector &dirPathParts, UString &fullPath);
  HRESULT GetTime(int index, PROPID propID, FILETIME &filetime, bool &filetimeIsDefined);
public:
  CArchiveExtractCallback():
      WriteModified(true),
      WriteCreated(false),
      WriteAccessed(false) 
      {}
  void Init(
      IInArchive *archiveHandler, 
      IFolderArchiveExtractCallback *extractCallback2,
      bool stdOutMode,
      const UString &directoryPath,
      NExtract::NPathMode::EEnum pathMode,
      NExtract::NOverwriteMode::EEnum overwriteMode,
      const UStringVector &removePathParts,
      const UString &itemDefaultName,
      const FILETIME &utcLastWriteTimeDefault, 
      UInt32 attributesDefault);

  UInt64 _numErrors;
};

#endif
