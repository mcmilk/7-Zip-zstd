// HashCalc.h

#ifndef __HASH_CALC_H
#define __HASH_CALC_H

#include "../../../Common/Wildcard.h"

#include "../../Common/CreateCoder.h"
#include "../../Common/MethodProps.h"

#include "Property.h"

const unsigned k_HashCalc_DigestSize_Max = 64;

const unsigned k_HashCalc_NumGroups = 4;

enum
{
  k_HashCalc_Index_Current,
  k_HashCalc_Index_DataSum,
  k_HashCalc_Index_NamesSum,
  k_HashCalc_Index_StreamsSum
};

struct CHasherState
{
  CMyComPtr<IHasher> Hasher;
  UString Name;
  UInt32 DigestSize;
  Byte Digests[k_HashCalc_NumGroups][k_HashCalc_DigestSize_Max];
};

struct IHashCalc
{
  virtual void InitForNewFile() = 0;
  virtual void Update(const void *data, UInt32 size) = 0;
  virtual void SetSize(UInt64 size) = 0;
  virtual void Final(bool isDir, bool isAltStream, const UString &path) = 0;
};

struct CHashBundle: public IHashCalc
{
  CObjectVector<CHasherState> Hashers;

  UInt64 NumFiles;
  UInt64 NumDirs;
  UInt64 NumAltStreams;
  UInt64 FilesSize;
  UInt64 AltStreamsSize;
  UInt64 NumErrors;

  UInt64 CurSize;

  HRESULT SetMethods(DECL_EXTERNAL_CODECS_LOC_VARS const UStringVector &methods);
  
  void Init()
  {
    NumFiles = NumDirs = NumAltStreams = FilesSize = AltStreamsSize = NumErrors = 0;
  }

  void InitForNewFile();
  void Update(const void *data, UInt32 size);
  void SetSize(UInt64 size);
  void Final(bool isDir, bool isAltStream, const UString &path);
};

#define INTERFACE_IHashCallbackUI(x) \
  virtual HRESULT StartScanning() x; \
  virtual HRESULT ScanProgress(UInt64 numFolders, UInt64 numFiles, UInt64 totalSize, const wchar_t *path, bool isDir) x; \
  virtual HRESULT CanNotFindError(const wchar_t *name, DWORD systemError) x; \
  virtual HRESULT FinishScanning() x; \
  virtual HRESULT SetNumFiles(UInt64 numFiles) x; \
  virtual HRESULT SetTotal(UInt64 size) x; \
  virtual HRESULT SetCompleted(const UInt64 *completeValue) x; \
  virtual HRESULT CheckBreak() x; \
  virtual HRESULT BeforeFirstFile(const CHashBundle &hb) x; \
  virtual HRESULT GetStream(const wchar_t *name, bool isFolder) x; \
  virtual HRESULT OpenFileError(const wchar_t *name, DWORD systemError) x; \
  virtual HRESULT SetOperationResult(UInt64 fileSize, const CHashBundle &hb, bool showHash) x; \
  virtual HRESULT AfterLastFile(const CHashBundle &hb) x; \

struct IHashCallbackUI
{
  INTERFACE_IHashCallbackUI(=0)
};

struct CHashOptions
{
  UStringVector Methods;
  bool OpenShareForWrite;
  bool StdInMode;
  bool AltStreamsMode;
  NWildcard::ECensorPathMode PathMode;
 
  CHashOptions(): StdInMode(false), OpenShareForWrite(false), AltStreamsMode(false), PathMode(NWildcard::k_RelatPath) {};
};

HRESULT HashCalc(
    DECL_EXTERNAL_CODECS_LOC_VARS
    const NWildcard::CCensor &censor,
    const CHashOptions &options,
    UString &errorInfo,
    IHashCallbackUI *callback);

void AddHashHexToString(char *dest, const Byte *data, UInt32 size);

#endif
