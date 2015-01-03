// ArchiveExtractCallback.cpp

#include "StdAfx.h"

#undef sprintf
#undef printf

#include "../../../Common/ComTry.h"
#include "../../../Common/StringConvert.h"
#include "../../../Common/Wildcard.h"

#include "../../../Windows/FileDir.h"
#include "../../../Windows/FileFind.h"
#include "../../../Windows/FileName.h"
#include "../../../Windows/PropVariant.h"
#include "../../../Windows/PropVariantConv.h"

#if defined(_WIN32) && !defined(UNDER_CE)  && !defined(_SFX)
#define _USE_SECURITY_CODE
#include "../../../Windows/SecurityUtils.h"
#endif

#include "../../Common/FilePathAutoRename.h"

#include "../Common/ExtractingFilePath.h"
#include "../Common/PropIDUtils.h"

#include "ArchiveExtractCallback.h"

using namespace NWindows;
using namespace NFile;
using namespace NDir;

static const char *kCantAutoRename = "Can not create file with auto name";
static const char *kCantRenameFile = "Can not rename existing file";
static const char *kCantDeleteOutputFile = "Can not delete output file";
static const char *kCantDeleteOutputDir = "Can not delete output folder";


#ifndef _SFX

STDMETHODIMP COutStreamWithHash::Write(const void *data, UInt32 size, UInt32 *processedSize)
{
  HRESULT result = S_OK;
  if (_stream)
    result = _stream->Write(data, size, &size);
  if (_calculate)
    _hash->Update(data, size);
  _size += size;
  if (processedSize)
    *processedSize = size;
  return result;
}

#endif

#ifdef _USE_SECURITY_CODE
bool InitLocalPrivileges()
{
  NSecurity::CAccessToken token;
  if (!token.OpenProcessToken(GetCurrentProcess(),
      TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES))
    return false;

  TOKEN_PRIVILEGES tp;
 
  tp.PrivilegeCount = 1;
  tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
  
  if  (!::LookupPrivilegeValue(NULL, SE_SECURITY_NAME, &tp.Privileges[0].Luid))
    return false;
  if (!token.AdjustPrivileges(&tp))
    return false;
  return (GetLastError() == ERROR_SUCCESS);
}
#endif

#ifdef SUPPORT_LINKS

int CHardLinkNode::Compare(const CHardLinkNode &a) const
{
  if (StreamId < a.StreamId) return -1;
  if (StreamId > a.StreamId) return 1;
  return MyCompare(INode, a.INode);
}

HRESULT Archive_Get_HardLinkNode(IInArchive *archive, UInt32 index, CHardLinkNode &h, bool &defined)
{
  h.INode = 0;
  h.StreamId = (UInt64)(Int64)-1;
  defined = false;
  {
    NCOM::CPropVariant prop;
    RINOK(archive->GetProperty(index, kpidINode, &prop));
    if (!ConvertPropVariantToUInt64(prop, h.INode))
      return S_OK;
  }
  {
    NCOM::CPropVariant prop;
    RINOK(archive->GetProperty(index, kpidStreamId, &prop));
    ConvertPropVariantToUInt64(prop, h.StreamId);
  }
  defined = true;
  return S_OK;
}


HRESULT CArchiveExtractCallback::PrepareHardLinks(const CRecordVector<UInt32> *realIndices)
{
  _hardLinks.Clear();

  if (!_arc->Ask_INode)
    return S_OK;
  
  IInArchive *archive = _arc->Archive;
  CRecordVector<CHardLinkNode> &hardIDs = _hardLinks.IDs;

  {
    UInt32 numItems;
    if (realIndices)
      numItems = realIndices->Size();
    else
    {
      RINOK(archive->GetNumberOfItems(&numItems));
    }

    for (UInt32 i = 0; i < numItems; i++)
    {
      CHardLinkNode h;
      bool defined;
      RINOK(Archive_Get_HardLinkNode(archive, realIndices ? (*realIndices)[i] : i, h, defined));
      if (defined)
        hardIDs.Add(h);
    }
  }
  
  hardIDs.Sort2();
  
  {
    // wee keep only items that have 2 or more items
    unsigned k = 0;
    unsigned numSame = 1;
    for (unsigned i = 1; i < hardIDs.Size(); i++)
    {
      if (hardIDs[i].Compare(hardIDs[i - 1]) != 0)
        numSame = 1;
      else if (++numSame == 2)
      {
        if (i - 1 != k)
          hardIDs[k] = hardIDs[i - 1];
        k++;
      }
    }
    hardIDs.DeleteFrom(k);
  }
  
  _hardLinks.PrepareLinks();
  return S_OK;
}

#endif

CArchiveExtractCallback::CArchiveExtractCallback():
    WriteCTime(true),
    WriteATime(true),
    WriteMTime(true),
    _multiArchives(false)
{
  LocalProgressSpec = new CLocalProgress();
  _localProgress = LocalProgressSpec;

  #ifdef _USE_SECURITY_CODE
  _saclEnabled = InitLocalPrivileges();
  #endif
}

void CArchiveExtractCallback::Init(
    const CExtractNtOptions &ntOptions,
    const NWildcard::CCensorNode *wildcardCensor,
    const CArc *arc,
    IFolderArchiveExtractCallback *extractCallback2,
    bool stdOutMode, bool testMode,
    const FString &directoryPath,
    const UStringVector &removePathParts,
    UInt64 packSize)
{
  _extractedFolderPaths.Clear();
  _extractedFolderIndices.Clear();
  
  #ifdef SUPPORT_LINKS
  _hardLinks.Clear();
  #endif

  _ntOptions = ntOptions;
  _wildcardCensor = wildcardCensor;

  _stdOutMode = stdOutMode;
  _testMode = testMode;
  _unpTotal = 1;
  _packTotal = packSize;

  _extractCallback2 = extractCallback2;
  _compressProgress.Release();
  _extractCallback2.QueryInterface(IID_ICompressProgressInfo, &_compressProgress);

  #ifndef _SFX

  _extractCallback2.QueryInterface(IID_IFolderExtractToStreamCallback, &ExtractToStreamCallback);
  if (ExtractToStreamCallback)
  {
    Int32 useStreams = 0;
    if (ExtractToStreamCallback->UseExtractToStream(&useStreams) != S_OK)
      useStreams = 0;
    if (useStreams == 0)
      ExtractToStreamCallback.Release();
  }
  
  #endif

  LocalProgressSpec->Init(extractCallback2, true);
  LocalProgressSpec->SendProgress = false;
 
  _removePathParts = removePathParts;
  _baseParentFolder = (UInt32)(Int32)-1;
  _use_baseParentFolder_mode = false;

  _arc = arc;
  _directoryPath = directoryPath;
  NName::NormalizeDirPathPrefix(_directoryPath);
  NDir::MyGetFullPathName(directoryPath, _directoryPathFull);
}

STDMETHODIMP CArchiveExtractCallback::SetTotal(UInt64 size)
{
  COM_TRY_BEGIN
  _unpTotal = size;
  if (!_multiArchives && _extractCallback2)
    return _extractCallback2->SetTotal(size);
  return S_OK;
  COM_TRY_END
}

static void NormalizeVals(UInt64 &v1, UInt64 &v2)
{
  const UInt64 kMax = (UInt64)1 << 31;
  while (v1 > kMax)
  {
    v1 >>= 1;
    v2 >>= 1;
  }
}

static UInt64 MyMultDiv64(UInt64 unpCur, UInt64 unpTotal, UInt64 packTotal)
{
  NormalizeVals(packTotal, unpTotal);
  NormalizeVals(unpCur, unpTotal);
  if (unpTotal == 0)
    unpTotal = 1;
  return unpCur * packTotal / unpTotal;
}

STDMETHODIMP CArchiveExtractCallback::SetCompleted(const UInt64 *completeValue)
{
  COM_TRY_BEGIN
  if (!_extractCallback2)
    return S_OK;

  if (_multiArchives)
  {
    if (completeValue != NULL)
    {
      UInt64 packCur = LocalProgressSpec->InSize + MyMultDiv64(*completeValue, _unpTotal, _packTotal);
      return _extractCallback2->SetCompleted(&packCur);
    }
  }
  return _extractCallback2->SetCompleted(completeValue);
  COM_TRY_END
}

STDMETHODIMP CArchiveExtractCallback::SetRatioInfo(const UInt64 *inSize, const UInt64 *outSize)
{
  COM_TRY_BEGIN
  return _localProgress->SetRatioInfo(inSize, outSize);
  COM_TRY_END
}

#define IS_LETTER_CHAR(c) ((c) >= 'a' && (c) <= 'z' || (c) >= 'A' && (c) <= 'Z')

static inline bool IsDriveName(const UString &s)
{
  return s.Len() == 2 && s[1] == ':' && IS_LETTER_CHAR(s[0]);
}

void CArchiveExtractCallback::CreateComplexDirectory(const UStringVector &dirPathParts, FString &fullPath)
{
  bool isAbsPath = false;
  
  if (!dirPathParts.IsEmpty())
  {
    const UString &s = dirPathParts[0];
    if (s.IsEmpty())
      isAbsPath = true;
    #ifdef _WIN32
    else
    {
      if (dirPathParts.Size() > 1 && IsDriveName(s))
        isAbsPath = true;
    }
    #endif
  }
  
  if (_pathMode == NExtract::NPathMode::kAbsPaths && isAbsPath)
    fullPath.Empty();
  else
    fullPath = _directoryPath;

  FOR_VECTOR (i, dirPathParts)
  {
    if (i > 0)
      fullPath += FCHAR_PATH_SEPARATOR;
    const UString &s = dirPathParts[i];
    fullPath += us2fs(s);
    #ifdef _WIN32
    if (_pathMode == NExtract::NPathMode::kAbsPaths)
      if (i == 0 && IsDriveName(s))
        continue;
    #endif
    CreateDir(fullPath);
  }
}

HRESULT CArchiveExtractCallback::GetTime(int index, PROPID propID, FILETIME &filetime, bool &filetimeIsDefined)
{
  filetimeIsDefined = false;
  NCOM::CPropVariant prop;
  RINOK(_arc->Archive->GetProperty(index, propID, &prop));
  if (prop.vt == VT_FILETIME)
  {
    filetime = prop.filetime;
    filetimeIsDefined = (filetime.dwHighDateTime != 0 || filetime.dwLowDateTime != 0);
  }
  else if (prop.vt != VT_EMPTY)
    return E_FAIL;
  return S_OK;
}

HRESULT CArchiveExtractCallback::GetUnpackSize()
{
  return _arc->GetItemSize(_index, _curSize, _curSizeDefined);
}

HRESULT CArchiveExtractCallback::SendMessageError(const char *message, const FString &path)
{
  return _extractCallback2->MessageError(
      UString(L"ERROR: ") +
      GetUnicodeString(message) + L": " + fs2us(path));
}

HRESULT CArchiveExtractCallback::SendMessageError2(const char *message, const FString &path1, const FString &path2)
{
  return _extractCallback2->MessageError(
      UString(L"ERROR: ") +
      GetUnicodeString(message) + UString(L": ") + fs2us(path1) + UString(L" : ") + fs2us(path2));
}

#ifndef _SFX

STDMETHODIMP CGetProp::GetProp(PROPID propID, PROPVARIANT *value)
{
  if (propID == kpidName)
  {
    COM_TRY_BEGIN
    NCOM::CPropVariant prop = Name.Ptr();
    prop.Detach(value);
    return S_OK;
    COM_TRY_END
  }
  return Arc->Archive->GetProperty(IndexInArc, propID, value);
}

#endif


#ifdef SUPPORT_LINKS

static UString GetDirPrefixOf(const UString &src)
{
  UString s = src;
  if (!s.IsEmpty())
  {
    if (s.Back() == WCHAR_PATH_SEPARATOR)
      s.DeleteBack();
    int pos = s.ReverseFind(WCHAR_PATH_SEPARATOR);
    s.DeleteFrom(pos + 1);
  }
  return s;
}

static bool IsSafePath(const UString &path)
{
  UStringVector parts;
  SplitPathToParts(path, parts);
  int level = 0;
  FOR_VECTOR(i, parts)
  {
    const UString &s = parts[i];
    if (s.IsEmpty())
      continue;
    if (s == L".")
      continue;
    if (s == L"..")
    {
      if (level <= 0)
        return false;
      level--;
    }
    else
      level++;
  }
  return level > 0;
}

#endif


STDMETHODIMP CArchiveExtractCallback::GetStream(UInt32 index, ISequentialOutStream **outStream, Int32 askExtractMode)
{
  COM_TRY_BEGIN

  *outStream = 0;

  #ifndef _SFX
  if (_hashStream)
    _hashStreamSpec->ReleaseStream();
  _hashStreamWasUsed = false;
  #endif

  _outFileStream.Release();

  _encrypted = false;
  _isSplit = false;
  _isAltStream = false;
  _curSize = 0;
  _curSizeDefined = false;
  _index = index;

  UString fullPath;

  IInArchive *archive = _arc->Archive;
  RINOK(_arc->GetItemPath(index, fullPath));
  RINOK(Archive_IsItem_Folder(archive, index, _fi.IsDir));

  _filePath = fullPath;

  {
    NCOM::CPropVariant prop;
    RINOK(archive->GetProperty(index, kpidPosition, &prop));
    if (prop.vt != VT_EMPTY)
    {
      if (prop.vt != VT_UI8)
        return E_FAIL;
      _position = prop.uhVal.QuadPart;
      _isSplit = true;
    }
  }

  #ifdef SUPPORT_LINKS
  
  bool isHardLink = false;
  bool isJunction = false;
  bool isRelative = false;

  UString linkPath;
  // RINOK(Archive_GetItemBoolProp(archive, index, kpidIsHardLink, isHardLink));
  // if (isHardLink)
  {
    NCOM::CPropVariant prop;
    RINOK(archive->GetProperty(index, kpidHardLink, &prop));
    if (prop.vt == VT_BSTR)
    {
      isHardLink = true;
      linkPath = prop.bstrVal;
      isRelative = false; // TAR: hard links are from root folder of archive
    }
    else if (prop.vt == VT_EMPTY)
    {
      // linkPath.Empty();
    }
    else
      return E_FAIL;
  }
  {
    NCOM::CPropVariant prop;
    RINOK(archive->GetProperty(index, kpidSymLink, &prop));
    if (prop.vt == VT_BSTR)
    {
      isHardLink = false;
      linkPath = prop.bstrVal;
      isRelative = true; // TAR: symbolic links are relative
    }
    else if (prop.vt == VT_EMPTY)
    {
      // linkPath.Empty();
    }
    else
      return E_FAIL;
  }

  bool isOkReparse = false;

  if (linkPath.IsEmpty() && _arc->GetRawProps)
  {
    const void *data;
    UInt32 dataSize;
    UInt32 propType;
    _arc->GetRawProps->GetRawProp(_index, kpidNtReparse, &data, &dataSize, &propType);
    if (dataSize != 0)
    {
      if (propType != NPropDataType::kRaw)
        return E_FAIL;
      UString s;
      CReparseAttr reparse;
      isOkReparse = reparse.Parse((const Byte *)data, dataSize);
      if (isOkReparse)
      {
        isHardLink = false;
        linkPath = reparse.GetPath();
        isJunction = reparse.IsMountPoint();
        isRelative = reparse.IsRelative();
        #ifndef _WIN32
        linkPath.Replace(WCHAR_PATH_SEPARATOR, '/', );
        #endif
      }
    }
  }

  if (!linkPath.IsEmpty())
  {
    #ifdef _WIN32
    linkPath.Replace('/', WCHAR_PATH_SEPARATOR);
    #endif
    
    for (;;)
    // while (NName::IsAbsolutePath(linkPath))
    {
      unsigned n = NName::GetRootPrefixSize(linkPath);
      if (n == 0)
        break;
      isRelative = false;
      linkPath.DeleteFrontal(n);
    }
  }

  if (!linkPath.IsEmpty() && !isRelative && _removePathParts.Size() != 0)
  {
    UStringVector pathParts;
    SplitPathToParts(linkPath, pathParts);
    bool badPrefix = false;
    FOR_VECTOR (i, _removePathParts)
    {
      if (CompareFileNames(_removePathParts[i], pathParts[i]) != 0)
      {
        badPrefix = true;
        break;
      }
    }
    if (!badPrefix)
      pathParts.DeleteFrontal(_removePathParts.Size());
    linkPath = MakePathNameFromParts(pathParts);
  }

  #endif
  
  RINOK(Archive_GetItemBoolProp(archive, index, kpidEncrypted, _encrypted));

  RINOK(GetUnpackSize());

  RINOK(Archive_IsItem_AltStream(archive, index, _isAltStream));

  if (!_ntOptions.AltStreams.Val && _isAltStream)
    return S_OK;

  if (_wildcardCensor)
  {
    if (!_wildcardCensor->CheckPath(_isAltStream, fullPath, !_fi.IsDir))
      return S_OK;
  }


  UStringVector pathParts;

  if (_use_baseParentFolder_mode)
  {
    int baseParent = _baseParentFolder;
    if (_pathMode == NExtract::NPathMode::kFullPaths ||
        _pathMode == NExtract::NPathMode::kAbsPaths)
      baseParent = -1;
    RINOK(_arc->GetItemPathToParent(index, baseParent, pathParts));
    if (_pathMode == NExtract::NPathMode::kNoPaths && !pathParts.IsEmpty())
      pathParts.DeleteFrontal(pathParts.Size() - 1);
  }
  else
  {
    SplitPathToParts(fullPath, pathParts);
    
    if (pathParts.IsEmpty())
      return E_FAIL;
    unsigned numRemovePathParts = 0;
    
    switch (_pathMode)
    {
      case NExtract::NPathMode::kCurPaths:
      {
        bool badPrefix = false;
        if (pathParts.Size() <= _removePathParts.Size())
          badPrefix = true;
        else
        {
          FOR_VECTOR (i, _removePathParts)
          {
            if (!_removePathParts[i].IsEqualToNoCase(pathParts[i]))
            {
              badPrefix = true;
              break;
            }
          }
        }
        if (badPrefix)
        {
          if (askExtractMode == NArchive::NExtract::NAskMode::kExtract && !_testMode)
            return E_FAIL;
        }
        else
          numRemovePathParts = _removePathParts.Size();
        break;
      }
      case NExtract::NPathMode::kNoPaths:
      {
        numRemovePathParts = pathParts.Size() - 1;
        break;
      }
      /*
      case NExtract::NPathMode::kFullPaths:
      case NExtract::NPathMode::kAbsPaths:
        break;
      */
    }
    
    pathParts.DeleteFrontal(numRemovePathParts);
  }

  #ifndef _SFX

  if (ExtractToStreamCallback)
  {
    if (!GetProp)
    {
      GetProp_Spec = new CGetProp;
      GetProp = GetProp_Spec;
    }
    GetProp_Spec->Arc = _arc;
    GetProp_Spec->IndexInArc = index;
    GetProp_Spec->Name = MakePathNameFromParts(pathParts);

    return ExtractToStreamCallback->GetStream7(GetProp_Spec->Name, _fi.IsDir, outStream, askExtractMode, GetProp);
  }

  #endif

  CMyComPtr<ISequentialOutStream> outStreamLoc;

if (askExtractMode == NArchive::NExtract::NAskMode::kExtract && !_testMode)
{
  if (_stdOutMode)
  {
    outStreamLoc = new CStdOutFileStream;
  }
  else
  {
    {
      NCOM::CPropVariant prop;
      RINOK(archive->GetProperty(index, kpidAttrib, &prop));
      if (prop.vt == VT_UI4)
      {
        _fi.Attrib = prop.ulVal;
        _fi.AttribDefined = true;
      }
      else if (prop.vt == VT_EMPTY)
        _fi.AttribDefined = false;
      else
        return E_FAIL;
    }

    RINOK(GetTime(index, kpidCTime, _fi.CTime, _fi.CTimeDefined));
    RINOK(GetTime(index, kpidATime, _fi.ATime, _fi.ATimeDefined));
    RINOK(GetTime(index, kpidMTime, _fi.MTime, _fi.MTimeDefined));

    bool isAnti = false;
    RINOK(_arc->IsItemAnti(index, isAnti));

    bool replace = _isAltStream ?
        _ntOptions.ReplaceColonForAltStream :
        !_ntOptions.WriteToAltStreamIfColon;

    if (_pathMode != NExtract::NPathMode::kAbsPaths)
      MakeCorrectPath(_directoryPath.IsEmpty(), pathParts, replace);
    Correct_IfEmptyLastPart(pathParts);
    UString processedPath = MakePathNameFromParts(pathParts);
    
    if (!isAnti)
    {
      if (!_fi.IsDir)
      {
        if (!pathParts.IsEmpty())
          pathParts.DeleteBack();
      }
    
      if (!pathParts.IsEmpty())
      {
        FString fullPathNew;
        CreateComplexDirectory(pathParts, fullPathNew);
        if (_fi.IsDir)
        {
          _extractedFolderPaths.Add(fullPathNew);
          _extractedFolderIndices.Add(index);
          SetDirTime(fullPathNew,
            (WriteCTime && _fi.CTimeDefined) ? &_fi.CTime : NULL,
            (WriteATime && _fi.ATimeDefined) ? &_fi.ATime : NULL,
            (WriteMTime && _fi.MTimeDefined) ? &_fi.MTime : (_arc->MTimeDefined ? &_arc->MTime : NULL));
        }
      }
    }


    FString fullProcessedPath = us2fs(processedPath);
    if (_pathMode != NExtract::NPathMode::kAbsPaths ||
        !NName::IsAbsolutePath(processedPath))
      fullProcessedPath = _directoryPath + fullProcessedPath;

    if (_fi.IsDir)
    {
      _diskFilePath = fullProcessedPath;
      if (isAnti)
        RemoveDir(_diskFilePath);
      #ifdef SUPPORT_LINKS
      if (linkPath.IsEmpty())
      #endif
        return S_OK;
    }
    else if (!_isSplit)
    {
    NFind::CFileInfo fileInfo;
    if (fileInfo.Find(fullProcessedPath))
    {
      switch (_overwriteMode)
      {
        case NExtract::NOverwriteMode::kSkip:
          return S_OK;
        case NExtract::NOverwriteMode::kAsk:
        {
          int slashPos = fullProcessedPath.ReverseFind(FTEXT('/'));
          #ifdef _WIN32
          int slash1Pos = fullProcessedPath.ReverseFind(FTEXT('\\'));
          slashPos = MyMax(slashPos, slash1Pos);
          #endif
          FString realFullProcessedPath = fullProcessedPath.Left(slashPos + 1) + fileInfo.Name;

          Int32 overwiteResult;
          RINOK(_extractCallback2->AskOverwrite(
              fs2us(realFullProcessedPath), &fileInfo.MTime, &fileInfo.Size, fullPath,
              _fi.MTimeDefined ? &_fi.MTime : NULL,
              _curSizeDefined ? &_curSize : NULL,
              &overwiteResult))

          switch (overwiteResult)
          {
            case NOverwriteAnswer::kCancel: return E_ABORT;
            case NOverwriteAnswer::kNo: return S_OK;
            case NOverwriteAnswer::kNoToAll: _overwriteMode = NExtract::NOverwriteMode::kSkip; return S_OK;
            case NOverwriteAnswer::kYes: break;
            case NOverwriteAnswer::kYesToAll: _overwriteMode = NExtract::NOverwriteMode::kOverwrite; break;
            case NOverwriteAnswer::kAutoRename: _overwriteMode = NExtract::NOverwriteMode::kRename; break;
            default:
              return E_FAIL;
          }
        }
      }
      if (_overwriteMode == NExtract::NOverwriteMode::kRename)
      {
        if (!AutoRenamePath(fullProcessedPath))
        {
          RINOK(SendMessageError(kCantAutoRename, fullProcessedPath));
          return E_FAIL;
        }
      }
      else if (_overwriteMode == NExtract::NOverwriteMode::kRenameExisting)
      {
        FString existPath = fullProcessedPath;
        if (!AutoRenamePath(existPath))
        {
          RINOK(SendMessageError(kCantAutoRename, fullProcessedPath));
          return E_FAIL;
        }
        // MyMoveFile can raname folders. So it's OK to use it folders too
        if (!MyMoveFile(fullProcessedPath, existPath))
        {
          RINOK(SendMessageError(kCantRenameFile, fullProcessedPath));
          return E_FAIL;
        }
      }
      else
      {
        if (fileInfo.IsDir())
        {
          // do we need to delete all files in folder?
          if (!RemoveDir(fullProcessedPath))
          {
            RINOK(SendMessageError(kCantDeleteOutputDir, fullProcessedPath));
            return S_OK;
          }
        }
        else if (!DeleteFileAlways(fullProcessedPath))
        {
          RINOK(SendMessageError(kCantDeleteOutputFile, fullProcessedPath));
          return S_OK;
          // return E_FAIL;
        }
      }
    }
    }
    _diskFilePath = fullProcessedPath;
    

    if (!isAnti)
    {
      #ifdef SUPPORT_LINKS

      if (!linkPath.IsEmpty())
      {
        #ifndef UNDER_CE

        UString relatPath;
        if (isRelative)
          relatPath = GetDirPrefixOf(_filePath);
        relatPath += linkPath;
        
        if (!IsSafePath(relatPath))
        {
          RINOK(SendMessageError("Dangerous link path was ignored", us2fs(relatPath)));
        }
        else
        {
          FString existPath;
          if (isHardLink || !isRelative)
          {
            if (!NName::GetFullPath(_directoryPathFull, us2fs(relatPath), existPath))
            {
              RINOK(SendMessageError("Incorrect path", us2fs(relatPath)));
            }
          }
          else
          {
            existPath = us2fs(linkPath);
          }
          
          if (!existPath.IsEmpty())
          {
            if (isHardLink)
            {
              if (!MyCreateHardLink(fullProcessedPath, existPath))
              {
                RINOK(SendMessageError2("Can not create hard link", fullProcessedPath, existPath));
                // return S_OK;
              }
            }
            else if (_ntOptions.SymLinks.Val)
            {
              // bool isSymLink = true; // = false for junction
              if (_fi.IsDir && !isRelative)
              {
                // if it's before Vista we use Junction Point
                // isJunction = true;
                // convertToAbs = true;
              }
              
              CByteBuffer data;
              if (FillLinkData(data, fs2us(existPath), !isJunction))
              {
                CReparseAttr attr;
                if (!attr.Parse(data, data.Size()))
                {
                  return E_FAIL; // "Internal conversion error";
                }
                
                if (!NFile::NIO::SetReparseData(fullProcessedPath, _fi.IsDir, data, (DWORD)data.Size()))
                {
                  RINOK(SendMessageError("Can not set reparse data", fullProcessedPath));
                }
              }
            }
          }
        }
        
        #endif
      }
      else
      #endif // SUPPORT_LINKS
      {
        bool needWriteFile = true;
        
        #ifdef SUPPORT_LINKS
        if (!_hardLinks.IDs.IsEmpty())
        {
          CHardLinkNode h;
          bool defined;
          RINOK(Archive_Get_HardLinkNode(archive, index, h, defined));
          if (defined)
          {
            {
              int linkIndex = _hardLinks.IDs.FindInSorted2(h);
              if (linkIndex >= 0)
              {
                FString &hl = _hardLinks.Links[linkIndex];
                if (hl.IsEmpty())
                  hl = fullProcessedPath;
                else
                {
                  if (!MyCreateHardLink(fullProcessedPath, hl))
                  {
                    RINOK(SendMessageError2("Can not create hard link", fullProcessedPath, hl));
                    return S_OK;
                  }
                  needWriteFile = false;
                }
              }
            }
          }
        }
        #endif
        
        if (needWriteFile)
        {
          _outFileStreamSpec = new COutFileStream;
          CMyComPtr<ISequentialOutStream> outStreamLoc(_outFileStreamSpec);
          if (!_outFileStreamSpec->Open(fullProcessedPath, _isSplit ? OPEN_ALWAYS: CREATE_ALWAYS))
          {
            // if (::GetLastError() != ERROR_FILE_EXISTS || !isSplit)
            {
              RINOK(SendMessageError("Can not open output file ", fullProcessedPath));
              return S_OK;
            }
          }
          if (_isSplit)
          {
            RINOK(_outFileStreamSpec->Seek(_position, STREAM_SEEK_SET, NULL));
          }
          _outFileStream = outStreamLoc;
        }
      }
    }
    
    outStreamLoc = _outFileStream;
  }
}

  #ifndef _SFX

  if (_hashStream)
  {
    if (askExtractMode == NArchive::NExtract::NAskMode::kExtract ||
        askExtractMode == NArchive::NExtract::NAskMode::kTest)
    {
      _hashStreamSpec->SetStream(outStreamLoc);
      outStreamLoc = _hashStream;
      _hashStreamSpec->Init(true);
      _hashStreamWasUsed = true;
    }
  }

  #endif

  if (outStreamLoc)
    *outStream = outStreamLoc.Detach();
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CArchiveExtractCallback::PrepareOperation(Int32 askExtractMode)
{
  COM_TRY_BEGIN

  #ifndef _SFX
  if (ExtractToStreamCallback)
    return ExtractToStreamCallback->PrepareOperation7(askExtractMode);
  #endif
  
  _extractMode = false;
  switch (askExtractMode)
  {
    case NArchive::NExtract::NAskMode::kExtract:
      if (_testMode)
        askExtractMode = NArchive::NExtract::NAskMode::kTest;
      else
        _extractMode = true;
      break;
  };
  return _extractCallback2->PrepareOperation(_filePath, _fi.IsDir,
      askExtractMode, _isSplit ? &_position: 0);
  COM_TRY_END
}

STDMETHODIMP CArchiveExtractCallback::SetOperationResult(Int32 operationResult)
{
  COM_TRY_BEGIN

  #ifndef _SFX
  if (ExtractToStreamCallback)
    return ExtractToStreamCallback->SetOperationResult7(operationResult, _encrypted);
  #endif

  #ifndef _SFX

  if (_hashStreamWasUsed)
  {
    _hashStreamSpec->_hash->Final(_fi.IsDir, _isAltStream, _filePath);
    _curSize = _hashStreamSpec->GetSize();
    _curSizeDefined = true;
    _hashStreamSpec->ReleaseStream();
    _hashStreamWasUsed = false;
  }

  #endif

  if (_outFileStream)
  {
    _outFileStreamSpec->SetTime(
        (WriteCTime && _fi.CTimeDefined) ? &_fi.CTime : NULL,
        (WriteATime && _fi.ATimeDefined) ? &_fi.ATime : NULL,
        (WriteMTime && _fi.MTimeDefined) ? &_fi.MTime : (_arc->MTimeDefined ? &_arc->MTime : NULL));
    _curSize = _outFileStreamSpec->ProcessedSize;
    _curSizeDefined = true;
    RINOK(_outFileStreamSpec->Close());
    _outFileStream.Release();
  }
  
  #ifdef _USE_SECURITY_CODE
  if (_ntOptions.NtSecurity.Val && _arc->GetRawProps)
  {
    const void *data;
    UInt32 dataSize;
    UInt32 propType;
    _arc->GetRawProps->GetRawProp(_index, kpidNtSecure, &data, &dataSize, &propType);
    if (dataSize != 0)
    {
      if (propType != NPropDataType::kRaw)
        return E_FAIL;
      if (CheckNtSecure((const Byte *)data, dataSize))
      {
        SECURITY_INFORMATION securInfo = DACL_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION;
        if (_saclEnabled)
          securInfo |= SACL_SECURITY_INFORMATION;
        ::SetFileSecurityW(fs2us(_diskFilePath), securInfo, (PSECURITY_DESCRIPTOR)(void *)data);
      }
    }
  }
  #endif

  if (!_curSizeDefined)
    GetUnpackSize();
  if (_curSizeDefined)
  {
    if (_isAltStream)
      AltStreams_UnpackSize += _curSize;
    else
      UnpackSize += _curSize;
  }
    
  if (_fi.IsDir)
    NumFolders++;
  else if (_isAltStream)
    NumAltStreams++;
  else
    NumFiles++;

  if (_extractMode && _fi.AttribDefined)
    SetFileAttrib(_diskFilePath, _fi.Attrib);
  RINOK(_extractCallback2->SetOperationResult(operationResult, _encrypted));
  return S_OK;
  COM_TRY_END
}

/*
STDMETHODIMP CArchiveExtractCallback::GetInStream(
    const wchar_t *name, ISequentialInStream **inStream)
{
  COM_TRY_BEGIN
  CInFileStream *inFile = new CInFileStream;
  CMyComPtr<ISequentialInStream> inStreamTemp = inFile;
  if (!inFile->Open(_srcDirectoryPrefix + name))
    return ::GetLastError();
  *inStream = inStreamTemp.Detach();
  return S_OK;
  COM_TRY_END
}
*/

STDMETHODIMP CArchiveExtractCallback::CryptoGetTextPassword(BSTR *password)
{
  COM_TRY_BEGIN
  if (!_cryptoGetTextPassword)
  {
    RINOK(_extractCallback2.QueryInterface(IID_ICryptoGetTextPassword,
        &_cryptoGetTextPassword));
  }
  return _cryptoGetTextPassword->CryptoGetTextPassword(password);
  COM_TRY_END
}


struct CExtrRefSortPair
{
  int Len;
  int Index;

  int Compare(const CExtrRefSortPair &a) const;
};

#define RINOZ(x) { int __tt = (x); if (__tt != 0) return __tt; }

int CExtrRefSortPair::Compare(const CExtrRefSortPair &a) const
{
  RINOZ(-MyCompare(Len, a.Len));
  return MyCompare(Index, a.Index);
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

HRESULT CArchiveExtractCallback::SetDirsTimes()
{
  CRecordVector<CExtrRefSortPair> pairs;
  pairs.ClearAndSetSize(_extractedFolderPaths.Size());
  unsigned i;
  
  for (i = 0; i < _extractedFolderPaths.Size(); i++)
  {
    CExtrRefSortPair &pair = pairs[i];
    pair.Index = i;
    pair.Len = GetNumSlashes(_extractedFolderPaths[i]);
  }
  
  pairs.Sort2();
  
  for (i = 0; i < pairs.Size(); i++)
  {
    int pairIndex = pairs[i].Index;
    int index = _extractedFolderIndices[pairIndex];

    FILETIME CTime;
    FILETIME ATime;
    FILETIME MTime;
  
    bool CTimeDefined;
    bool ATimeDefined;
    bool MTimeDefined;

    RINOK(GetTime(index, kpidCTime, CTime, CTimeDefined));
    RINOK(GetTime(index, kpidATime, ATime, ATimeDefined));
    RINOK(GetTime(index, kpidMTime, MTime, MTimeDefined));

    // printf("\n%S", _extractedFolderPaths[pairIndex]);
    SetDirTime(_extractedFolderPaths[pairIndex],
      (WriteCTime && CTimeDefined) ? &CTime : NULL,
      (WriteATime && ATimeDefined) ? &ATime : NULL,
      (WriteMTime && MTimeDefined) ? &MTime : (_arc->MTimeDefined ? &_arc->MTime : NULL));
  }
  return S_OK;
}
