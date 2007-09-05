// ArchiveExtractCallback.h

#ifndef __ARCHIVEEXTRACTCALLBACK_H
#define __ARCHIVEEXTRACTCALLBACK_H

#include "../../Archive/IArchive.h"
#include "IFileExtractCallback.h"

#include "Common/MyString.h"
#include "Common/MyCom.h"

#include "../../Common/FileStreams.h"
#include "../../Common/ProgressUtils.h"
#include "../../IPassword.h"

#include "ExtractMode.h"

class CArchiveExtractCallback: 
  public IArchiveExtractCallback,
  // public IArchiveVolumeExtractCallback,
  public ICryptoGetTextPassword,
  public ICompressProgressInfo,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP2(ICryptoGetTextPassword, ICompressProgressInfo)
  // COM_INTERFACE_ENTRY(IArchiveVolumeExtractCallback)

  // IProgress
  STDMETHOD(SetTotal)(UInt64 size);
  STDMETHOD(SetCompleted)(const UInt64 *completeValue);
  STDMETHOD(SetRatioInfo)(const UInt64 *inSize, const UInt64 *outSize);

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
  CMyComPtr<ICompressProgressInfo> _compressProgress;
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

  UInt64 _curSize;
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
      WriteAccessed(false),
      _multiArchives(false)
  {
    LocalProgressSpec = new CLocalProgress();
    _localProgress = LocalProgressSpec;
  }

  CLocalProgress *LocalProgressSpec;
  CMyComPtr<ICompressProgressInfo> _localProgress;
  UInt64 _packTotal;
  UInt64 _unpTotal;

  bool _multiArchives;
  UInt64 NumFolders;
  UInt64 NumFiles;
  UInt64 UnpackSize;
  
  void InitForMulti(bool multiArchives, 
      NExtract::NPathMode::EEnum pathMode,
      NExtract::NOverwriteMode::EEnum overwriteMode) 
  { 
    _multiArchives = multiArchives; NumFolders = NumFiles = UnpackSize = 0; 
    _pathMode = pathMode;
    _overwriteMode = overwriteMode;
  }

  void Init(
      IInArchive *archiveHandler, 
      IFolderArchiveExtractCallback *extractCallback2,
      bool stdOutMode,
      const UString &directoryPath,
      const UStringVector &removePathParts,
      const UString &itemDefaultName,
      const FILETIME &utcLastWriteTimeDefault, 
      UInt32 attributesDefault,
      UInt64 packSize);

  UInt64 _numErrors;
};

#endif
