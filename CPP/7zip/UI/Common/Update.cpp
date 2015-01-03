// Update.cpp

#include "StdAfx.h"

#include "Update.h"

#include "../../../Common/IntToString.h"
#include "../../../Common/StringConvert.h"

#include "../../../Windows/DLL.h"
#include "../../../Windows/FileDir.h"
#include "../../../Windows/FileFind.h"
#include "../../../Windows/FileName.h"
#include "../../../Windows/PropVariant.h"
#include "../../../Windows/PropVariantConv.h"
#include "../../../Windows/TimeUtils.h"

#include "../../Common/FileStreams.h"
#include "../../Common/LimitedStreams.h"

#include "../../Compress/CopyCoder.h"

#include "../Common/DirItem.h"
#include "../Common/EnumDirItems.h"
#include "../Common/OpenArchive.h"
#include "../Common/UpdateProduce.h"

#include "EnumDirItems.h"
#include "SetProperties.h"
#include "TempFiles.h"
#include "UpdateCallback.h"

static const char *kUpdateIsNotSupoorted =
  "update operations are not supported for this archive";

using namespace NWindows;
using namespace NCOM;
using namespace NFile;
using namespace NDir;
using namespace NName;

static CFSTR kTempFolderPrefix = FTEXT("7zE");


static bool DeleteEmptyFolderAndEmptySubFolders(const FString &path)
{
  NFind::CFileInfo fileInfo;
  FString pathPrefix = path + FCHAR_PATH_SEPARATOR;
  {
    NFind::CEnumerator enumerator(pathPrefix + FCHAR_ANY_MASK);
    while (enumerator.Next(fileInfo))
    {
      if (fileInfo.IsDir())
        if (!DeleteEmptyFolderAndEmptySubFolders(pathPrefix + fileInfo.Name))
          return false;
    }
  }
  /*
  // we don't need clear read-only for folders
  if (!MySetFileAttributes(path, 0))
    return false;
  */
  return RemoveDir(path);
}


using namespace NUpdateArchive;

class COutMultiVolStream:
  public IOutStream,
  public CMyUnknownImp
{
  unsigned _streamIndex; // required stream
  UInt64 _offsetPos; // offset from start of _streamIndex index
  UInt64 _absPos;
  UInt64 _length;

  struct CAltStreamInfo
  {
    COutFileStream *StreamSpec;
    CMyComPtr<IOutStream> Stream;
    FString Name;
    UInt64 Pos;
    UInt64 RealSize;
  };
  CObjectVector<CAltStreamInfo> Streams;
public:
  // CMyComPtr<IArchiveUpdateCallback2> VolumeCallback;
  CRecordVector<UInt64> Sizes;
  FString Prefix;
  CTempFiles *TempFiles;

  void Init()
  {
    _streamIndex = 0;
    _offsetPos = 0;
    _absPos = 0;
    _length = 0;
  }

  bool SetMTime(const FILETIME *mTime);
  HRESULT Close();

  MY_UNKNOWN_IMP1(IOutStream)

  STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);
  STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition);
  STDMETHOD(SetSize)(UInt64 newSize);
};

// static NSynchronization::CCriticalSection g_TempPathsCS;

HRESULT COutMultiVolStream::Close()
{
  HRESULT res = S_OK;
  FOR_VECTOR (i, Streams)
  {
    COutFileStream *s = Streams[i].StreamSpec;
    if (s)
    {
      HRESULT res2 = s->Close();
      if (res2 != S_OK)
        res = res2;
    }
  }
  return res;
}

bool COutMultiVolStream::SetMTime(const FILETIME *mTime)
{
  bool res = true;
  FOR_VECTOR (i, Streams)
  {
    COutFileStream *s = Streams[i].StreamSpec;
    if (s)
      if (!s->SetMTime(mTime))
        res = false;
  }
  return res;
}

STDMETHODIMP COutMultiVolStream::Write(const void *data, UInt32 size, UInt32 *processedSize)
{
  if (processedSize != NULL)
    *processedSize = 0;
  while (size > 0)
  {
    if (_streamIndex >= Streams.Size())
    {
      CAltStreamInfo altStream;

      FChar temp[16];
      ConvertUInt32ToString(_streamIndex + 1, temp);
      FString res = temp;
      while (res.Len() < 3)
        res = FString(FTEXT('0')) + res;
      FString name = Prefix + res;
      altStream.StreamSpec = new COutFileStream;
      altStream.Stream = altStream.StreamSpec;
      if (!altStream.StreamSpec->Create(name, false))
        return ::GetLastError();
      {
        // NSynchronization::CCriticalSectionLock lock(g_TempPathsCS);
        TempFiles->Paths.Add(name);
      }

      altStream.Pos = 0;
      altStream.RealSize = 0;
      altStream.Name = name;
      Streams.Add(altStream);
      continue;
    }
    CAltStreamInfo &altStream = Streams[_streamIndex];

    unsigned index = _streamIndex;
    if (index >= Sizes.Size())
      index = Sizes.Size() - 1;
    UInt64 volSize = Sizes[index];

    if (_offsetPos >= volSize)
    {
      _offsetPos -= volSize;
      _streamIndex++;
      continue;
    }
    if (_offsetPos != altStream.Pos)
    {
      // CMyComPtr<IOutStream> outStream;
      // RINOK(altStream.Stream.QueryInterface(IID_IOutStream, &outStream));
      RINOK(altStream.Stream->Seek(_offsetPos, STREAM_SEEK_SET, NULL));
      altStream.Pos = _offsetPos;
    }

    UInt32 curSize = (UInt32)MyMin((UInt64)size, volSize - altStream.Pos);
    UInt32 realProcessed;
    RINOK(altStream.Stream->Write(data, curSize, &realProcessed));
    data = (void *)((Byte *)data + realProcessed);
    size -= realProcessed;
    altStream.Pos += realProcessed;
    _offsetPos += realProcessed;
    _absPos += realProcessed;
    if (_absPos > _length)
      _length = _absPos;
    if (_offsetPos > altStream.RealSize)
      altStream.RealSize = _offsetPos;
    if (processedSize != NULL)
      *processedSize += realProcessed;
    if (altStream.Pos == volSize)
    {
      _streamIndex++;
      _offsetPos = 0;
    }
    if (realProcessed == 0 && curSize != 0)
      return E_FAIL;
    break;
  }
  return S_OK;
}

STDMETHODIMP COutMultiVolStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition)
{
  if (seekOrigin >= 3)
    return STG_E_INVALIDFUNCTION;
  switch (seekOrigin)
  {
    case STREAM_SEEK_SET: _absPos = offset; break;
    case STREAM_SEEK_CUR: _absPos += offset; break;
    case STREAM_SEEK_END: _absPos = _length + offset; break;
  }
  _offsetPos = _absPos;
  if (newPosition != NULL)
    *newPosition = _absPos;
  _streamIndex = 0;
  return S_OK;
}

STDMETHODIMP COutMultiVolStream::SetSize(UInt64 newSize)
{
  if (newSize < 0)
    return E_INVALIDARG;
  unsigned i = 0;
  while (i < Streams.Size())
  {
    CAltStreamInfo &altStream = Streams[i++];
    if ((UInt64)newSize < altStream.RealSize)
    {
      RINOK(altStream.Stream->SetSize(newSize));
      altStream.RealSize = newSize;
      break;
    }
    newSize -= altStream.RealSize;
  }
  while (i < Streams.Size())
  {
    {
      CAltStreamInfo &altStream = Streams.Back();
      altStream.Stream.Release();
      DeleteFileAlways(altStream.Name);
    }
    Streams.DeleteBack();
  }
  _offsetPos = _absPos;
  _streamIndex = 0;
  _length = newSize;
  return S_OK;
}

void CArchivePath::ParseFromPath(const UString &path, EArcNameMode mode)
{
  OriginalPath = path;
  
  SplitPathToParts_2(path, Prefix, Name);
  
  if (mode == k_ArcNameMode_Add)
    return;
  if (mode == k_ArcNameMode_Exact)
  {
    BaseExtension.Empty();
    return;
  }
  
  int dotPos = Name.ReverseFind(L'.');
  if (dotPos < 0)
    return;
  if ((unsigned)dotPos == Name.Len() - 1)
  {
    Name.DeleteBack();
    BaseExtension.Empty();
    return;
  }
  const UString ext = Name.Ptr(dotPos + 1);
  if (BaseExtension.IsEqualToNoCase(ext))
  {
    BaseExtension = ext;
    Name.DeleteFrom(dotPos);
  }
  else
    BaseExtension.Empty();
}

UString CArchivePath::GetFinalPath() const
{
  UString path = GetPathWithoutExt();
  if (!BaseExtension.IsEmpty())
    path += UString(L'.') + BaseExtension;
  return path;
}

UString CArchivePath::GetFinalVolPath() const
{
  UString path = GetPathWithoutExt();
  if (!BaseExtension.IsEmpty())
    path += UString(L'.') + VolExtension;
  return path;
}

FString CArchivePath::GetTempPath() const
{
  FString path = TempPrefix + us2fs(Name);
  if (!BaseExtension.IsEmpty())
    path += FString(FTEXT('.')) + us2fs(BaseExtension);
  path += FTEXT(".tmp");
  path += TempPostfix;
  return path;
}

static const wchar_t *kDefaultArcType = L"7z";
static const wchar_t *kDefaultArcExt = L"7z";
static const wchar_t *kSFXExtension =
  #ifdef _WIN32
    L"exe";
  #else
    L"";
  #endif

bool CUpdateOptions::InitFormatIndex(const CCodecs *codecs,
    const CObjectVector<COpenType> &types, const UString &arcPath)
{
  if (types.Size() > 1)
    return false;
  // int arcTypeIndex = -1;
  if (types.Size() != 0)
  {
    MethodMode.Type = types[0];
    MethodMode.Type_Defined = true;
  }
  if (MethodMode.Type.FormatIndex < 0)
  {
    // MethodMode.Type = -1;
    MethodMode.Type = COpenType();
    if (ArcNameMode != k_ArcNameMode_Add)
    {
      MethodMode.Type.FormatIndex = codecs->FindFormatForArchiveName(arcPath);
      if (MethodMode.Type.FormatIndex >= 0)
        MethodMode.Type_Defined = true;
    }
  }
  return true;
}

bool CUpdateOptions::SetArcPath(const CCodecs *codecs, const UString &arcPath)
{
  UString typeExt;
  int formatIndex = MethodMode.Type.FormatIndex;
  if (formatIndex < 0)
  {
    typeExt = kDefaultArcExt;
  }
  else
  {
    const CArcInfoEx &arcInfo = codecs->Formats[formatIndex];
    if (!arcInfo.UpdateEnabled)
      return false;
    typeExt = arcInfo.GetMainExt();
  }
  UString ext = typeExt;
  if (SfxMode)
    ext = kSFXExtension;
  ArchivePath.BaseExtension = ext;
  ArchivePath.VolExtension = typeExt;
  ArchivePath.ParseFromPath(arcPath, ArcNameMode);
  FOR_VECTOR (i, Commands)
  {
    CUpdateArchiveCommand &uc = Commands[i];
    uc.ArchivePath.BaseExtension = ext;
    uc.ArchivePath.VolExtension = typeExt;
    uc.ArchivePath.ParseFromPath(uc.UserArchivePath, ArcNameMode);
  }
  return true;
}

/*
struct CUpdateProduceCallbackImp: public IUpdateProduceCallback
{
  const CObjectVector<CArcItem> *_arcItems;
  IUpdateCallbackUI *_callback;
  
  CUpdateProduceCallbackImp(const CObjectVector<CArcItem> *a,
      IUpdateCallbackUI *callback): _arcItems(a), _callback(callback) {}
  virtual HRESULT ShowDeleteFile(int arcIndex);
};

HRESULT CUpdateProduceCallbackImp::ShowDeleteFile(int arcIndex)
{
  return _callback->ShowDeleteFile((*_arcItems)[arcIndex].Name);
}
*/

bool CRenamePair::Prepare()
{
  if (RecursedType != NRecursedType::kNonRecursed)
    return false;
  if (!WildcardParsing)
    return true;
  return !DoesNameContainWildcard(OldName);
}

extern bool g_CaseSensitive;

static int CompareTwoNames(const wchar_t *s1, const wchar_t *s2)
{
  for (int i = 0;; i++)
  {
    wchar_t c1 = s1[i];
    wchar_t c2 = s2[i];
    if (c1 == 0 || c2 == 0)
      return i;
    if (c1 == c2)
      continue;
    if (!g_CaseSensitive && (MyCharUpper(c1) == MyCharUpper(c2)))
      continue;
    if (IsCharDirLimiter(c1) && IsCharDirLimiter(c2))
      continue;
    return i;
  }
}

bool CRenamePair::GetNewPath(bool isFolder, const UString &src, UString &dest) const
{
  int num = CompareTwoNames(OldName, src);
  if (OldName[num] == 0)
  {
    if (src[num] != 0 && !IsCharDirLimiter(src[num]) && num != 0 && !IsCharDirLimiter(src[num - 1]))
      return false;
  }
  else
  {
    // OldName[num] != 0
    // OldName = "1\1a.txt"
    // src = "1"

    if (!isFolder ||
        src[num] != 0 ||
        !IsCharDirLimiter(OldName[num]) ||
        OldName[num + 1] != 0)
      return false;
  }
  dest = NewName + src.Ptr(num);
  return true;
}

static int GetReverseSlashPos(const UString &name)
{
  int slashPos = name.ReverseFind(L'/');
  #ifdef _WIN32
  int slash1Pos = name.ReverseFind(L'\\');
  slashPos = MyMax(slashPos, slash1Pos);
  #endif
  return slashPos;
}

static HRESULT Compress(
    const CUpdateOptions &options,
    CCodecs *codecs,
    const CActionSet &actionSet,
    const CArc *arc,
    CArchivePath &archivePath,
    const CObjectVector<CArcItem> &arcItems,
    Byte *processedItemsStatuses,
    const CDirItems &dirItems,
    const CDirItem *parentDirItem,
    CTempFiles &tempFiles,
    CUpdateErrorInfo &errorInfo,
    IUpdateCallbackUI *callback)
{
  CMyComPtr<IOutArchive> outArchive;
  int formatIndex = options.MethodMode.Type.FormatIndex;
  if (arc)
  {
    formatIndex = arc->FormatIndex;
    if (formatIndex < 0)
      return E_NOTIMPL;
    CMyComPtr<IInArchive> archive2 = arc->Archive;
    HRESULT result = archive2.QueryInterface(IID_IOutArchive, &outArchive);
    if (result != S_OK)
      throw kUpdateIsNotSupoorted;
  }
  else
  {
    RINOK(codecs->CreateOutArchive(formatIndex, outArchive));

    #ifdef EXTERNAL_CODECS
    {
      CMyComPtr<ISetCompressCodecsInfo> setCompressCodecsInfo;
      outArchive.QueryInterface(IID_ISetCompressCodecsInfo, (void **)&setCompressCodecsInfo);
      if (setCompressCodecsInfo)
      {
        RINOK(setCompressCodecsInfo->SetCompressCodecsInfo(codecs));
      }
    }
    #endif
  }
  if (outArchive == 0)
    throw kUpdateIsNotSupoorted;
  
  NFileTimeType::EEnum fileTimeType;
  UInt32 value;
  RINOK(outArchive->GetFileTimeType(&value));

  switch (value)
  {
    case NFileTimeType::kWindows:
    case NFileTimeType::kUnix:
    case NFileTimeType::kDOS:
      fileTimeType = (NFileTimeType::EEnum)value;
      break;
    default:
      return E_FAIL;
  }

  {
    const CArcInfoEx &arcInfo = codecs->Formats[formatIndex];
    if (options.AltStreams.Val && !arcInfo.Flags_AltStreams())
      return E_NOTIMPL;
    if (options.NtSecurity.Val && !arcInfo.Flags_NtSecure())
      return E_NOTIMPL;
  }

  CRecordVector<CUpdatePair2> updatePairs2;

  UStringVector newNames;

  if (options.RenamePairs.Size() != 0)
  {
    FOR_VECTOR (i, arcItems)
    {
      const CArcItem &ai = arcItems[i];
      bool needRename = false;
      UString dest;
      if (ai.Censored)
      {
        FOR_VECTOR (j, options.RenamePairs)
        {
          const CRenamePair &rp = options.RenamePairs[j];
          if (rp.GetNewPath(ai.IsDir, ai.Name, dest))
          {
            needRename = true;
            break;
          }
          if (ai.IsAltStream)
          {
            int colonPos = ai.Name.ReverseFind(':');
            int slashPosPos = GetReverseSlashPos(ai.Name);
            if (colonPos > slashPosPos)
            {
              UString mainName = ai.Name.Left(colonPos);
              /*
              actually we must improve that code to support cases
              with folder renaming like: rn arc dir1\ dir2\
              */
              if (rp.GetNewPath(false, mainName, dest))
              {
                needRename = true;
                dest += ':';
                dest += ai.Name.Ptr(colonPos + 1);
                break;
              }
            }
          }
        }
      }
      CUpdatePair2 up2;
      up2.SetAs_NoChangeArcItem(ai.IndexInServer);
      if (needRename)
      {
        up2.NewProps = true;
        RINOK(arc->IsItemAnti(i, up2.IsAnti));
        up2.NewNameIndex = newNames.Add(dest);
      }
      updatePairs2.Add(up2);
    }
  }
  else
  {
    CRecordVector<CUpdatePair> updatePairs;
    GetUpdatePairInfoList(dirItems, arcItems, fileTimeType, updatePairs); // must be done only once!!!
    // CUpdateProduceCallbackImp upCallback(&arcItems, callback);
    UpdateProduce(updatePairs, actionSet, updatePairs2, NULL /* &upCallback */);
  }

  UInt32 numFiles = 0;
  FOR_VECTOR (i, updatePairs2)
    if (updatePairs2[i].NewData)
      numFiles++;
  
  RINOK(callback->SetNumFiles(numFiles));
  
  CArchiveUpdateCallback *updateCallbackSpec = new CArchiveUpdateCallback;
  CMyComPtr<IArchiveUpdateCallback> updateCallback(updateCallbackSpec);
  
  updateCallbackSpec->ShareForWrite = options.OpenShareForWrite;
  updateCallbackSpec->StdInMode = options.StdInMode;
  updateCallbackSpec->Callback = callback;

  if (arc)
  {
    // we set Archive to allow to transfer GetProperty requests back to DLL.
    updateCallbackSpec->Archive = arc->Archive;
    updateCallbackSpec->GetRawProps = arc->GetRawProps;
    updateCallbackSpec->GetRootProps = arc->GetRootProps;
  }
  
  updateCallbackSpec->DirItems = &dirItems;
  updateCallbackSpec->ParentDirItem = parentDirItem;

  updateCallbackSpec->StoreNtSecurity = options.NtSecurity.Val;
  updateCallbackSpec->StoreHardLinks = options.HardLinks.Val;
  updateCallbackSpec->StoreSymLinks = options.SymLinks.Val;

  updateCallbackSpec->ArcItems = &arcItems;
  updateCallbackSpec->UpdatePairs = &updatePairs2;

  updateCallbackSpec->ProcessedItemsStatuses = processedItemsStatuses;

  if (options.RenamePairs.Size() != 0)
    updateCallbackSpec->NewNames = &newNames;

  CMyComPtr<IOutStream> outSeekStream;
  CMyComPtr<ISequentialOutStream> outStream;

  if (!options.StdOutMode)
  {
    FString dirPrefix;
    if (!GetOnlyDirPrefix(us2fs(archivePath.GetFinalPath()), dirPrefix))
      throw 1417161;
    CreateComplexDir(dirPrefix);
  }

  COutFileStream *outStreamSpec = NULL;
  COutMultiVolStream *volStreamSpec = NULL;

  if (options.VolumesSizes.Size() == 0)
  {
    if (options.StdOutMode)
      outStream = new CStdOutFileStream;
    else
    {
      outStreamSpec = new COutFileStream;
      outSeekStream = outStreamSpec;
      outStream = outSeekStream;
      bool isOK = false;
      FString realPath;
      for (int i = 0; i < (1 << 16); i++)
      {
        if (archivePath.Temp)
        {
          if (i > 0)
          {
            FChar s[16];
            ConvertUInt32ToString(i, s);
            archivePath.TempPostfix = s;
          }
          realPath = archivePath.GetTempPath();
        }
        else
          realPath = us2fs(archivePath.GetFinalPath());
        if (outStreamSpec->Create(realPath, false))
        {
          tempFiles.Paths.Add(realPath);
          isOK = true;
          break;
        }
        if (::GetLastError() != ERROR_FILE_EXISTS)
          break;
        if (!archivePath.Temp)
          break;
      }
      if (!isOK)
      {
        errorInfo.SystemError = ::GetLastError();
        errorInfo.FileName = realPath;
        errorInfo.Message = L"7-Zip cannot open file";
        return E_FAIL;
      }
    }
  }
  else
  {
    if (options.StdOutMode)
      return E_FAIL;
    if (arc && arc->GetGlobalOffset() > 0)
      return E_NOTIMPL;
      
    volStreamSpec = new COutMultiVolStream;
    outSeekStream = volStreamSpec;
    outStream = outSeekStream;
    volStreamSpec->Sizes = options.VolumesSizes;
    volStreamSpec->Prefix = us2fs(archivePath.GetFinalVolPath() + L".");
    volStreamSpec->TempFiles = &tempFiles;
    volStreamSpec->Init();

    /*
    updateCallbackSpec->VolumesSizes = volumesSizes;
    updateCallbackSpec->VolName = archivePath.Prefix + archivePath.Name;
    if (!archivePath.VolExtension.IsEmpty())
      updateCallbackSpec->VolExt = UString(L'.') + archivePath.VolExtension;
    */
  }

  RINOK(SetProperties(outArchive, options.MethodMode.Properties));

  if (options.SfxMode)
  {
    CInFileStream *sfxStreamSpec = new CInFileStream;
    CMyComPtr<IInStream> sfxStream(sfxStreamSpec);
    if (!sfxStreamSpec->Open(options.SfxModule))
    {
      errorInfo.SystemError = ::GetLastError();
      errorInfo.Message = L"7-Zip cannot open SFX module";
      errorInfo.FileName = options.SfxModule;
      return E_FAIL;
    }

    CMyComPtr<ISequentialOutStream> sfxOutStream;
    COutFileStream *outStreamSpec = NULL;
    if (options.VolumesSizes.Size() == 0)
      sfxOutStream = outStream;
    else
    {
      outStreamSpec = new COutFileStream;
      sfxOutStream = outStreamSpec;
      FString realPath = us2fs(archivePath.GetFinalPath());
      if (!outStreamSpec->Create(realPath, false))
      {
        errorInfo.SystemError = ::GetLastError();
        errorInfo.FileName = realPath;
        errorInfo.Message = L"7-Zip cannot open file";
        return E_FAIL;
      }
    }
    RINOK(NCompress::CopyStream(sfxStream, sfxOutStream, NULL));
    if (outStreamSpec)
    {
      RINOK(outStreamSpec->Close());
    }
  }

  CMyComPtr<ISequentialOutStream> tailStream;

  if (options.SfxMode || !arc || arc->ArcStreamOffset == 0)
    tailStream = outStream;
  else
  {
    // Int64 globalOffset = arc->GetGlobalOffset();
    RINOK(arc->InStream->Seek(0, STREAM_SEEK_SET, NULL));
    RINOK(NCompress::CopyStream_ExactSize(arc->InStream, outStream, arc->ArcStreamOffset, NULL));
    if (options.StdOutMode)
      tailStream = outStream;
    else
    {
      CTailOutStream *tailStreamSpec = new CTailOutStream;
      tailStream = tailStreamSpec;
      tailStreamSpec->Stream = outSeekStream;
      tailStreamSpec->Offset = arc->ArcStreamOffset;
      tailStreamSpec->Init();
    }
  }


  HRESULT result = outArchive->UpdateItems(tailStream, updatePairs2.Size(), updateCallback);
  callback->Finilize();
  RINOK(result);


  if (options.SetArcMTime)
  {
    FILETIME ft;
    ft.dwLowDateTime = 0;
    ft.dwHighDateTime = 0;
    FOR_VECTOR (i, updatePairs2)
    {
      CUpdatePair2 &pair2 = updatePairs2[i];
      const FILETIME *ft2 = NULL;
      if (pair2.NewProps && pair2.DirIndex >= 0)
        ft2 = &dirItems.Items[pair2.DirIndex].MTime;
      else if (pair2.UseArcProps && pair2.ArcIndex >= 0)
        ft2 = &arcItems[pair2.ArcIndex].MTime;
      if (ft2)
      {
        if (::CompareFileTime(&ft, ft2) < 0)
          ft = *ft2;
      }
    }
    if (ft.dwLowDateTime != 0 || ft.dwHighDateTime != 0)
    {
      if (outStreamSpec)
        outStreamSpec->SetMTime(&ft);
      else if (volStreamSpec)
        volStreamSpec->SetMTime(&ft);;
    }
  }

  if (outStreamSpec)
    result = outStreamSpec->Close();
  else if (volStreamSpec)
    result = volStreamSpec->Close();
  return result;
}

static HRESULT EnumerateInArchiveItems(
    // bool storeStreamsMode,
    const NWildcard::CCensor &censor,
    const CArc &arc,
    CObjectVector<CArcItem> &arcItems)
{
  arcItems.Clear();
  UInt32 numItems;
  IInArchive *archive = arc.Archive;
  RINOK(archive->GetNumberOfItems(&numItems));
  arcItems.ClearAndReserve(numItems);
  for (UInt32 i = 0; i < numItems; i++)
  {
    CArcItem ai;

    RINOK(arc.GetItemPath(i, ai.Name));
    RINOK(Archive_IsItem_Folder(archive, i, ai.IsDir));
    RINOK(Archive_IsItem_AltStream(archive, i, ai.IsAltStream));
    /*
    if (!storeStreamsMode && ai.IsAltStream)
      continue;
    */
    ai.Censored = censor.CheckPath(ai.IsAltStream, ai.Name, !ai.IsDir);
    RINOK(arc.GetItemMTime(i, ai.MTime, ai.MTimeDefined));
    RINOK(arc.GetItemSize(i, ai.Size, ai.SizeDefined));

    {
      CPropVariant prop;
      RINOK(archive->GetProperty(i, kpidTimeType, &prop));
      if (prop.vt == VT_UI4)
      {
        ai.TimeType = (int)(NFileTimeType::EEnum)prop.ulVal;
        switch (ai.TimeType)
        {
          case NFileTimeType::kWindows:
          case NFileTimeType::kUnix:
          case NFileTimeType::kDOS:
            break;
          default:
            return E_FAIL;
        }
      }
    }

    ai.IndexInServer = i;
    arcItems.AddInReserved(ai);
  }
  return S_OK;
}

struct CEnumDirItemUpdateCallback: public IEnumDirItemCallback
{
  IUpdateCallbackUI2 *Callback;
  HRESULT ScanProgress(UInt64 numFolders, UInt64 numFiles, UInt64 totalSize, const wchar_t *path, bool isDir)
  {
    return Callback->ScanProgress(numFolders, numFiles, totalSize, path, isDir);
  }
};

#if defined(_WIN32) && !defined(UNDER_CE)

#include <mapi.h>

#endif

struct CRefSortPair
{
  int Len;
  int Index;
};

#define RINOZ(x) { int __tt = (x); if (__tt != 0) return __tt; }

static int CompareRefSortPair(const CRefSortPair *a1, const CRefSortPair *a2, void *)
{
  RINOZ(-MyCompare(a1->Len, a2->Len));
  return MyCompare(a1->Index, a2->Index);
}

static int GetNumSlashes(const FChar *s)
{
  for (int numSlashes = 0;;)
  {
    FChar c = *s++;
    if (c == 0)
      return numSlashes;
    if (
        #ifdef _WIN32
        c == FTEXT('\\') ||
        #endif
        c == FTEXT('/'))
      numSlashes++;
  }
}

#ifdef _WIN32
void ConvertToLongNames(NWildcard::CCensor &censor);
#endif

HRESULT UpdateArchive(
    CCodecs *codecs,
    const CObjectVector<COpenType> &types,
    const UString &cmdArcPath2,
    NWildcard::CCensor &censor,
    CUpdateOptions &options,
    CUpdateErrorInfo &errorInfo,
    IOpenCallbackUI *openCallback,
    IUpdateCallbackUI2 *callback,
    bool needSetPath)
{
  if (options.StdOutMode && options.EMailMode)
    return E_FAIL;

  if (types.Size() > 1)
    return E_NOTIMPL;

  bool renameMode = !options.RenamePairs.IsEmpty();
  if (renameMode)
  {
    if (options.Commands.Size() != 1)
      return E_FAIL;
  }

  if (options.DeleteAfterCompressing)
  {
    if (options.Commands.Size() != 1)
      return E_NOTIMPL;
    const CActionSet &as = options.Commands[0].ActionSet;
    for (int i = 2; i < NPairState::kNumValues; i++)
      if (as.StateActions[i] != NPairAction::kCompress)
        return E_NOTIMPL;
  }

  censor.AddPathsToCensor(options.PathMode);
  #ifdef _WIN32
  ConvertToLongNames(censor);
  #endif
  censor.ExtendExclude();

  
  if (options.VolumesSizes.Size() > 0 && (options.EMailMode /* || options.SfxMode */))
    return E_NOTIMPL;

  if (options.SfxMode)
  {
    CProperty property;
    property.Name = L"rsfx";
    property.Value = L"on";
    options.MethodMode.Properties.Add(property);
    if (options.SfxModule.IsEmpty())
    {
      errorInfo.Message = L"SFX file is not specified";
      return E_FAIL;
    }
    bool found = false;
    if (options.SfxModule.Find(FCHAR_PATH_SEPARATOR) < 0)
    {
      const FString fullName = NDLL::GetModuleDirPrefix() + options.SfxModule;
      if (NFind::DoesFileExist(fullName))
      {
        options.SfxModule = fullName;
        found = true;
      }
    }
    if (!found)
    {
      if (!NFind::DoesFileExist(options.SfxModule))
      {
        errorInfo.SystemError = ::GetLastError();
        errorInfo.Message = L"7-Zip cannot find specified SFX module";
        errorInfo.FileName = options.SfxModule;
        return E_FAIL;
      }
    }
  }

  CArchiveLink arcLink;

  
  if (needSetPath)
  {
    if (!options.InitFormatIndex(codecs, types, cmdArcPath2) ||
        !options.SetArcPath(codecs, cmdArcPath2))
      return E_NOTIMPL;
  }
  UString arcPath = options.ArchivePath.GetFinalPath();

  if (cmdArcPath2.IsEmpty())
  {
    if (options.MethodMode.Type.FormatIndex < 0)
      throw "type of archive is not specified";
  }
  else
  {
    NFind::CFileInfo fi;
    if (!fi.Find(us2fs(arcPath)))
    {
      if (renameMode)
        throw "can't find archive";;
      if (options.MethodMode.Type.FormatIndex < 0)
      {
        if (!options.SetArcPath(codecs, cmdArcPath2))
          return E_NOTIMPL;
      }
    }
    else
    {
      if (fi.IsDir())
        throw "there is no such archive";
      if (fi.IsDevice)
        return E_NOTIMPL;
      if (options.VolumesSizes.Size() > 0)
        return E_NOTIMPL;
      CObjectVector<COpenType> types;
      // change it.
      if (options.MethodMode.Type_Defined)
        types.Add(options.MethodMode.Type);
      // We need to set Properties to open archive only in some cases (WIM archives).

      CIntVector excl;
      COpenOptions op;
      #ifndef _SFX
      op.props = &options.MethodMode.Properties;
      #endif
      op.codecs = codecs;
      op.types = &types;
      op.excludedFormats = &excl;
      op.stdInMode = false;
      op.stream = NULL;
      op.filePath = arcPath;

      HRESULT result = arcLink.Open2(op, openCallback);

      if (result == E_ABORT)
        return result;
      
      const wchar_t *errorArcType = NULL;
      if (arcLink.NonOpen_ErrorInfo.ErrorFormatIndex > 0)
        errorArcType = codecs->Formats[arcLink.NonOpen_ErrorInfo.ErrorFormatIndex].Name;
      RINOK(callback->OpenResult(arcPath, result, errorArcType));
      /*
      if (result == S_FALSE)
        return E_FAIL;
      */
      RINOK(result);
      if (arcLink.VolumePaths.Size() > 1)
      {
        errorInfo.SystemError = (DWORD)E_NOTIMPL;
        errorInfo.Message = L"Updating for multivolume archives is not implemented";
        return E_NOTIMPL;
      }
      
      CArc &arc = arcLink.Arcs.Back();
      arc.MTimeDefined = !fi.IsDevice;
      arc.MTime = fi.MTime;

      if (arc.ErrorInfo.ThereIsTail)
      {
        errorInfo.SystemError = (DWORD)E_NOTIMPL;
        errorInfo.Message = L"There is some data block after the end of the archive";
        return E_NOTIMPL;
      }
      if (options.MethodMode.Type.FormatIndex < 0)
      {
        options.MethodMode.Type.FormatIndex = arcLink.GetArc()->FormatIndex;
        if (!options.SetArcPath(codecs, cmdArcPath2))
          return E_NOTIMPL;
      }
    }
  }

  if (options.MethodMode.Type.FormatIndex < 0)
  {
    options.MethodMode.Type.FormatIndex = codecs->FindFormatForArchiveType(kDefaultArcType);
    if (options.MethodMode.Type.FormatIndex < 0)
      return E_NOTIMPL;
  }

  bool thereIsInArchive = arcLink.IsOpen;
  if (!thereIsInArchive && renameMode)
    return E_FAIL;

  CDirItems dirItems;
  CDirItem parentDirItem;
  CDirItem *parentDirItem_Ptr = NULL;
  
  /*
  FStringVector requestedPaths;
  FStringVector *requestedPaths_Ptr = NULL;
  if (options.DeleteAfterCompressing)
    requestedPaths_Ptr = &requestedPaths;
  */

  if (options.StdInMode)
  {
    CDirItem di;
    di.Name = options.StdInFileName;
    di.Size = (UInt64)(Int64)-1;
    di.Attrib = 0;
    NTime::GetCurUtcFileTime(di.MTime);
    di.CTime = di.ATime = di.MTime;
    dirItems.Items.Add(di);
  }
  else
  {
    bool needScanning = false;
    if (!renameMode)
    FOR_VECTOR (i, options.Commands)
      if (options.Commands[i].ActionSet.NeedScanning())
        needScanning = true;
    if (needScanning)
    {
      CEnumDirItemUpdateCallback enumCallback;
      enumCallback.Callback = callback;
      RINOK(callback->StartScanning());

      dirItems.SymLinks = options.SymLinks.Val;

      #if defined(_WIN32) && !defined(UNDER_CE)
      dirItems.ReadSecure = options.NtSecurity.Val;
      #endif

      dirItems.ScanAltStreams = options.AltStreams.Val;
      HRESULT res = EnumerateItems(censor,
          options.PathMode,
          options.AddPathPrefix,
          dirItems, &enumCallback);
      FOR_VECTOR (i, dirItems.ErrorPaths)
      {
        RINOK(callback->CanNotFindError(fs2us(dirItems.ErrorPaths[i]), dirItems.ErrorCodes[i]));
      }
      if (res != S_OK)
      {
        if (res != E_ABORT)
          errorInfo.Message = L"Scanning error";
        return res;
      }
      RINOK(callback->FinishScanning());

      if (censor.Pairs.Size() == 1)
      {
        NFind::CFileInfo fi;
        FString prefix = us2fs(censor.Pairs[0].Prefix) + FTEXT(".");
        // UString prefix = censor.Pairs[0].Prefix;
        /*
        if (prefix.Back() == WCHAR_PATH_SEPARATOR)
        {
          prefix.DeleteBack();
        }
        */
        if (fi.Find(prefix))
          if (fi.IsDir())
          {
            parentDirItem.Size = fi.Size;
            parentDirItem.CTime = fi.CTime;
            parentDirItem.ATime = fi.ATime;
            parentDirItem.MTime = fi.MTime;
            parentDirItem.Attrib = fi.Attrib;
            parentDirItem_Ptr = &parentDirItem;

            int secureIndex = -1;
            #if defined(_WIN32) && !defined(UNDER_CE)
            if (options.NtSecurity.Val)
              dirItems.AddSecurityItem(prefix, secureIndex);
            #endif
            parentDirItem.SecureIndex = secureIndex;

            parentDirItem_Ptr = &parentDirItem;
          }
      }
    }
  }

  FString tempDirPrefix;
  bool usesTempDir = false;
  
  #ifdef _WIN32
  CTempDir tempDirectory;
  if (options.EMailMode && options.EMailRemoveAfter)
  {
    tempDirectory.Create(kTempFolderPrefix);
    tempDirPrefix = tempDirectory.GetPath();
    NormalizeDirPathPrefix(tempDirPrefix);
    usesTempDir = true;
  }
  #endif

  CTempFiles tempFiles;

  bool createTempFile = false;

  if (!options.StdOutMode && options.UpdateArchiveItself)
  {
    CArchivePath &ap = options.Commands[0].ArchivePath;
    ap = options.ArchivePath;
    // if ((archive != 0 && !usesTempDir) || !options.WorkingDir.IsEmpty())
    if ((thereIsInArchive || !options.WorkingDir.IsEmpty()) && !usesTempDir && options.VolumesSizes.Size() == 0)
    {
      createTempFile = true;
      ap.Temp = true;
      if (!options.WorkingDir.IsEmpty())
        ap.TempPrefix = options.WorkingDir;
      else
        ap.TempPrefix = us2fs(ap.Prefix);
      NormalizeDirPathPrefix(ap.TempPrefix);
    }
  }

  unsigned i;
  for (i = 0; i < options.Commands.Size(); i++)
  {
    CArchivePath &ap = options.Commands[i].ArchivePath;
    if (usesTempDir)
    {
      // Check it
      ap.Prefix = fs2us(tempDirPrefix);
      // ap.Temp = true;
      // ap.TempPrefix = tempDirPrefix;
    }
    if (!options.StdOutMode &&
        (i > 0 || !createTempFile))
    {
      const FString path = us2fs(ap.GetFinalPath());
      if (NFind::DoesFileOrDirExist(path))
      {
        errorInfo.SystemError = 0;
        errorInfo.Message = L"The file already exists";
        errorInfo.FileName = path;
        return E_FAIL;
      }
    }
  }

  CObjectVector<CArcItem> arcItems;
  if (thereIsInArchive)
  {
    RINOK(EnumerateInArchiveItems(
      // options.StoreAltStreams,
      censor, arcLink.Arcs.Back(), arcItems));
  }

  /*
  FStringVector processedFilePaths;
  FStringVector *processedFilePaths_Ptr = NULL;
  if (options.DeleteAfterCompressing)
    processedFilePaths_Ptr = &processedFilePaths;
  */

  CByteBuffer processedItems;
  if (options.DeleteAfterCompressing)
  {
    unsigned num = dirItems.Items.Size();
    processedItems.Alloc(num);
    for (i = 0; i < num; i++)
      processedItems[i] = 0;
  }

  for (i = 0; i < options.Commands.Size(); i++)
  {
    const CArc *arc = thereIsInArchive ? arcLink.GetArc() : 0;
    // IInArchiveExtra *archiveExtra = thereIsInArchive ? arcLink.GetArchiveExtra() : 0;
    // IArchiveGetRootProps *archiveGetRootProps = thereIsInArchive ? arcLink.GetArchiveGetRootProps() : 0;
    CUpdateArchiveCommand &command = options.Commands[i];
    UString name;
    bool isUpdating;
    if (options.StdOutMode)
    {
      name = L"stdout";
      isUpdating = arc != 0;
    }
    else
    {
      name = command.ArchivePath.GetFinalPath();
      isUpdating = (i == 0 && options.UpdateArchiveItself && arc != 0);
    }
    RINOK(callback->StartArchive(name, isUpdating))

    RINOK(Compress(options,
        codecs,
        command.ActionSet,
        arc,
        command.ArchivePath,
        arcItems,
        options.DeleteAfterCompressing ? (Byte *)processedItems : NULL,

        dirItems,
        parentDirItem_Ptr,

        tempFiles,
        errorInfo, callback));

    RINOK(callback->FinishArchive());
  }


  if (thereIsInArchive)
  {
    RINOK(arcLink.Close());
    arcLink.Release();
  }

  tempFiles.Paths.Clear();
  if (createTempFile)
  {
    try
    {
      CArchivePath &ap = options.Commands[0].ArchivePath;
      const FString &tempPath = ap.GetTempPath();
      if (thereIsInArchive)
        if (!DeleteFileAlways(us2fs(arcPath)))
        {
          errorInfo.SystemError = ::GetLastError();
          errorInfo.Message = L"7-Zip cannot delete the file";
          errorInfo.FileName = us2fs(arcPath);
          return E_FAIL;
        }
      if (!MyMoveFile(tempPath, us2fs(arcPath)))
      {
        errorInfo.SystemError = ::GetLastError();
        errorInfo.Message = L"7-Zip cannot move the file";
        errorInfo.FileName = tempPath;
        errorInfo.FileName2 = us2fs(arcPath);
        return E_FAIL;
      }
    }
    catch(...)
    {
      throw;
    }
  }


  #if defined(_WIN32) && !defined(UNDER_CE)
  if (options.EMailMode)
  {
    NDLL::CLibrary mapiLib;
    if (!mapiLib.Load(FTEXT("Mapi32.dll")))
    {
      errorInfo.SystemError = ::GetLastError();
      errorInfo.Message = L"7-Zip cannot load Mapi32.dll";
      return E_FAIL;
    }

    /*
    LPMAPISENDDOCUMENTS fnSend = (LPMAPISENDDOCUMENTS)mapiLib.GetProc("MAPISendDocuments");
    if (fnSend == 0)
    {
      errorInfo.SystemError = ::GetLastError();
      errorInfo.Message = L"7-Zip cannot find MAPISendDocuments function";
      return E_FAIL;
    }
    */
    LPMAPISENDMAIL sendMail = (LPMAPISENDMAIL)mapiLib.GetProc("MAPISendMail");
    if (sendMail == 0)
    {
      errorInfo.SystemError = ::GetLastError();
      errorInfo.Message = L"7-Zip cannot find MAPISendMail function";
      return E_FAIL;
    }

    FStringVector fullPaths;
    unsigned i;
    for (i = 0; i < options.Commands.Size(); i++)
    {
      CArchivePath &ap = options.Commands[i].ArchivePath;
      FString arcPath;
      if (!MyGetFullPathName(us2fs(ap.GetFinalPath()), arcPath))
      {
        errorInfo.SystemError = ::GetLastError();
        errorInfo.Message = L"GetFullPathName error";
        return E_FAIL;
      }
      fullPaths.Add(arcPath);
    }
    CCurrentDirRestorer curDirRestorer;
    for (i = 0; i < fullPaths.Size(); i++)
    {
      UString arcPath = fs2us(fullPaths[i]);
      UString fileName = ExtractFileNameFromPath(arcPath);
      AString path = GetAnsiString(arcPath);
      AString name = GetAnsiString(fileName);
      // Warning!!! MAPISendDocuments function changes Current directory
      // fnSend(0, ";", (LPSTR)(LPCSTR)path, (LPSTR)(LPCSTR)name, 0);

      MapiFileDesc f;
      memset(&f, 0, sizeof(f));
      f.nPosition = 0xFFFFFFFF;
      f.lpszPathName = (char *)(const char *)path;
      f.lpszFileName = (char *)(const char *)name;
      
      MapiMessage m;
      memset(&m, 0, sizeof(m));
      m.nFileCount = 1;
      m.lpFiles = &f;
      
      const AString addr = GetAnsiString(options.EMailAddress);
      MapiRecipDesc rec;
      if (!addr.IsEmpty())
      {
        memset(&rec, 0, sizeof(rec));
        rec.ulRecipClass = MAPI_TO;
        rec.lpszAddress = (char *)(const char *)addr;
        m.nRecipCount = 1;
        m.lpRecips = &rec;
      }
      
      sendMail((LHANDLE)0, 0, &m, MAPI_DIALOG, 0);
    }
  }
  #endif

  if (options.DeleteAfterCompressing)
  {
    CRecordVector<CRefSortPair> pairs;
    FStringVector foldersNames;
    for (i = 0; i < dirItems.Items.Size(); i++)
    {
      const CDirItem &dirItem = dirItems.Items[i];
      FString phyPath = us2fs(dirItems.GetPhyPath(i));
      if (dirItem.IsDir())
      {
        CRefSortPair pair;
        pair.Index = i;
        pair.Len = GetNumSlashes(phyPath);
        pairs.Add(pair);
      }
      else
      {
        if (processedItems[i] != 0 || dirItem.Size == 0)
        {
          DeleteFileAlways(phyPath);
        }
        else
        {
          // file was skipped
          /*
          errorInfo.SystemError = 0;
          errorInfo.Message = L"file was not processed";
          errorInfo.FileName = phyPath;
          return E_FAIL;
          */
        }
      }
    }

    pairs.Sort(CompareRefSortPair, NULL);
    for (i = 0; i < pairs.Size(); i++)
    {
      FString phyPath = us2fs(dirItems.GetPhyPath(pairs[i].Index));
      if (NFind::DoesDirExist(phyPath))
      {
        // printf("delete %S\n", phyPath);
        RemoveDir(phyPath);
      }
    }
  }
  return S_OK;
}
