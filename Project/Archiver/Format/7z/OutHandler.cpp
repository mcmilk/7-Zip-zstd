// 7z/OutHandler.cpp

#include "StdAfx.h"

#include "Handler.h"
#include "OutEngine.h"
#include "Common/StringConvert.h"
#include "UpdateMain.h"

#include "Windows/PropVariant.h"
#include "Windows/Time.h"
#include "Windows/COMTry.h"

#include "RegistryInfo.h"

using namespace NArchive;
using namespace N7z;

using namespace NWindows;
using namespace NCOM;
using namespace NTime;

#ifdef COMPRESS_LZMA
static NArchive::N7z::CMethodID k_LZMA = { { 0x3, 0x1, 0x1 }, 3 };
#endif

#ifdef COMPRESS_PPMD
static NArchive::N7z::CMethodID k_PPMD = { { 0x3, 0x4, 0x1 }, 3 };
#endif

#ifdef COMPRESS_BCJ_X86
static NArchive::N7z::CMethodID k_BCJ_X86 = { { 0x3, 0x3, 0x1, 0x3 }, 4 };
#endif

#ifdef COMPRESS_BCJ2
static NArchive::N7z::CMethodID k_BCJ2 = { { 0x3, 0x3, 0x1, 0x1B }, 4 };
#endif

#ifdef COMPRESS_COPY
static NArchive::N7z::CMethodID k_Copy = { { 0x0 }, 1 };
#endif

#ifdef COMPRESS_DEFLATE
static NArchive::N7z::CMethodID k_Deflate = { { 0x4, 0x1, 0x8 }, 3 };
#endif

#ifdef COMPRESS_BZIP2
static NArchive::N7z::CMethodID k_BZip2 = { { 0x4, 0x2, 0x2 }, 3 };
#endif

const char *kLZMAMethodName = "LZMA";
// const char *kDeflateMethodName = "Deflate";

const UINT32 kDicSizeForX = (1 << 22);

const char *kDefaultMethodName = kLZMAMethodName;

const char *kDefaultMatchFinder = "BT234";

static bool IsLZMAMethod(const AString &aMethodName)
{
  return (aMethodName.CompareNoCase(kLZMAMethodName) == 0);
}

static bool IsLZMethod(const AString &aMethodName)
{
  return (IsLZMAMethod(aMethodName) 
      // || aMethodName.CompareNoCase(kDeflateMethodName) == 0
      );
}

STDMETHODIMP CHandler::GetFileTimeType(UINT32 *aType)
{
  *aType = NFileTimeType::kWindows;
  return S_OK;
}


// it's work only fopr non-solid archives

STDMETHODIMP CHandler::DeleteItems(IOutStream *anOutStream, 
    const UINT32* anIndexes, UINT32 aNumItems, IUpdateCallBack *anUpdateCallBack)
{
  COM_TRY_BEGIN
  CRecordVector<bool> aCompressStatuses;
  CRecordVector<UINT32> aCopyIndexes;
  int anIndex = 0;
  for(int i = 0; i < m_Database.m_NumUnPackStreamsVector.Size(); i++)
  {
    if (m_Database.m_NumUnPackStreamsVector[i] != 1)
      return E_FAIL;
  }
  for(i = 0; i < m_Database.m_Files.Size(); i++)
  {
    // bool aCopyMode = true;
    if(anIndex < aNumItems && i == anIndexes[anIndex])
      anIndex++;
    else
    {
      aCompressStatuses.Add(false);
      aCopyIndexes.Add(i);
    }
  }
  CCompressionMethodMode aMethodMode, aHeaderMethod;
  RETURN_IF_NOT_S_OK(SetCompressionMethod(aMethodMode, aHeaderMethod));
  UpdateMain(m_Database, aCompressStatuses,
      CObjectVector<CUpdateItemInfo>(), aCopyIndexes,
      anOutStream, m_InStream, &m_Database.m_ArchiveInfo, 
      NULL, (m_CompressHeaders ? &aHeaderMethod: 0), 
      anUpdateCallBack, false);
  return S_OK;
  COM_TRY_END
}

struct CNameToPropID
{
  PROPID PropID;
  VARTYPE VarType;
  const char *Name;
  bool CoderProperties;
};

CNameToPropID g_NameToPropID[] = 
{
  { NEncodedStreamProperies::kOrder, VT_UI4, "O", true  },
  { NEncodedStreamProperies::kPosStateBits, VT_UI4, "PB", true  },
  { NEncodedStreamProperies::kLitContextBits, VT_UI4, "LC", true  },
  { NEncodedStreamProperies::kLitPosBits, VT_UI4, "LP", true  },

  { NEncodingProperies::kNumPasses, VT_UI4, "Pass", false },
  { NEncodingProperies::kNumFastBytes, VT_UI4, "fb", false }
};

bool ConvertProperty(PROPVARIANT aPropFrom, VARTYPE aVarType, 
    NCOM::CPropVariant & aPropTo)
{
  if (aVarType == aPropFrom.vt)
  {
    aPropTo = aPropFrom;
    return true;
  }
  if (aVarType == VT_UI1)
  {
    if(aPropFrom.vt == VT_UI4)
    {
      UINT32 aValue = aPropFrom.ulVal;
      if (aValue > 0xFF)
        return false;
      aPropTo = BYTE(aValue);
      return true;
    }
  }
  return false;
}
    
const kNumNameToPropIDItems = sizeof(g_NameToPropID) / sizeof(g_NameToPropID[0]);

int FindPropIdFromStringName(const AString &aName)
{
  for (int i = 0; i < kNumNameToPropIDItems; i++)
    if (aName.CompareNoCase(g_NameToPropID[i].Name) == 0)
      return i;
  return -1;
}

HRESULT CHandler::SetCompressionMethod(CCompressionMethodMode &aMethodMode,
    CCompressionMethodMode &aHeaderMethod)
{
  #ifndef EXCLUDE_COM
  CObjectVector<NRegistryInfo::CMethodInfo2> aMethodInfoVector;
  if (!NRegistryInfo::EnumerateAllMethods(aMethodInfoVector))
    return E_FAIL;
  #endif
 

  if (m_Methods.IsEmpty())
  {
    COneMethodInfo anOneMethodInfo;
    anOneMethodInfo.MethodName = kDefaultMethodName;
    anOneMethodInfo.MatchFinderIsDefined = false;
    m_Methods.Add(anOneMethodInfo);
  }

  for(int i = 0; i < m_Methods.Size(); i++)
  {
    COneMethodInfo &anOneMethodInfo = m_Methods[i];
    if (anOneMethodInfo.MethodName.IsEmpty())
      anOneMethodInfo.MethodName = kDefaultMethodName;

    if (IsLZMethod(anOneMethodInfo.MethodName))
    {
      if (!anOneMethodInfo.MatchFinderIsDefined)
      {
        anOneMethodInfo.MatchFinderName = GetSystemString(kDefaultMatchFinder);
        anOneMethodInfo.MatchFinderIsDefined = true;
      }
      if (IsLZMAMethod(anOneMethodInfo.MethodName))
      {
        for (int j = 0; j < anOneMethodInfo.CoderProperties.Size(); j++)
          if (anOneMethodInfo.CoderProperties[j].PropID == NEncodedStreamProperies::kDictionarySize)
            break;
        if (j == anOneMethodInfo.CoderProperties.Size())
        {
          CProperty aProperty;
          aProperty.PropID = NEncodedStreamProperies::kDictionarySize;
          aProperty.Value = m_DefaultDicSize;
          anOneMethodInfo.CoderProperties.Add(aProperty);
        }
      }
    }
    CMethodFull aMethodFull;
    aMethodFull.MethodInfoEx.NumInStreams = 1;
    aMethodFull.MethodInfoEx.NumOutStreams = 1;

    bool aDefined = false;

    #ifdef COMPRESS_LZMA
    if (anOneMethodInfo.MethodName.CompareNoCase("LZMA") == 0)
    {
      aDefined = true;
      aMethodFull.MethodInfoEx.MethodID = k_LZMA;
    }
    #endif

    #ifdef COMPRESS_PPMD
    if (anOneMethodInfo.MethodName.CompareNoCase("PPMD") == 0)
    {
      aDefined = true;
      aMethodFull.MethodInfoEx.MethodID = k_PPMD;
    }
    #endif

    #ifdef COMPRESS_BCJ_X86
    if (anOneMethodInfo.MethodName.CompareNoCase("BCJ") == 0)
    {
      aDefined = true;
      aMethodFull.MethodInfoEx.MethodID = k_BCJ_X86;
    }
    #endif

    #ifdef COMPRESS_BCJ2
    if (anOneMethodInfo.MethodName.CompareNoCase("BCJ2") == 0)
    {
      aDefined = true;
      aMethodFull.MethodInfoEx.MethodID = k_BCJ2;
      aMethodFull.MethodInfoEx.NumInStreams = 4;
      aMethodFull.MethodInfoEx.NumOutStreams = 1;
    }
    #endif

    #ifdef COMPRESS_DEFLATE
    if (anOneMethodInfo.MethodName.CompareNoCase("Deflate") == 0)
    {
      aDefined = true;
      aMethodFull.MethodInfoEx.MethodID = k_Deflate;
    }
    #endif

    #ifdef COMPRESS_BZIP2
    if (anOneMethodInfo.MethodName.CompareNoCase("BZip2") == 0)
    {
      aDefined = true;
      aMethodFull.MethodInfoEx.MethodID = k_BZip2;
    }
    #endif

    #ifdef COMPRESS_COPY
    if (anOneMethodInfo.MethodName.CompareNoCase("Copy") == 0)
    {
      aDefined = true;
      aMethodFull.MethodInfoEx.MethodID = k_Copy;
    }

    #endif
    
    #ifdef EXCLUDE_COM
    
    if (aDefined)
    {
  
      aMethodFull.CoderProperties = anOneMethodInfo.CoderProperties;
      aMethodFull.EncoderProperties = anOneMethodInfo.EncoderProperties;
      aMethodFull.MatchFinderIsDefined = anOneMethodInfo.MatchFinderIsDefined;
      aMethodFull.MatchFinderName = anOneMethodInfo.MatchFinderName;
      aMethodMode.Methods.Add(aMethodFull);
      continue;
    }
    
    #else

    int j;
    for (j = 0; j < aMethodInfoVector.Size(); j++)
      if (aMethodInfoVector[j].Name.CompareNoCase(anOneMethodInfo.MethodName) == 0)
        break;
    if (j == aMethodInfoVector.Size())
      return E_FAIL;
    const NRegistryInfo::CMethodInfo2 &aMethodInfo = aMethodInfoVector[j];
    if (!aMethodInfo.EncoderIsAssigned)
      return E_FAIL;

    aMethodFull.MethodInfoEx.MethodID = aMethodInfo.MethodID;
    aMethodFull.MethodInfoEx.NumInStreams = aMethodInfo.NumInStreams;
    aMethodFull.MethodInfoEx.NumOutStreams = aMethodInfo.NumOutStreams;

    aMethodFull.EncoderClassID = aMethodInfo.Encoder;
    aMethodFull.CoderProperties = anOneMethodInfo.CoderProperties;
    aMethodFull.EncoderProperties = anOneMethodInfo.EncoderProperties;
    aMethodFull.MatchFinderIsDefined = anOneMethodInfo.MatchFinderIsDefined;
    if (anOneMethodInfo.MatchFinderIsDefined)
    {
      NRegistryInfo::CMatchFinderInfo aMatchFinderInfo;
      if (!NRegistryInfo::GetMatchFinder(anOneMethodInfo.MatchFinderName, aMatchFinderInfo))
        return E_INVALIDARG;
      aMethodFull.MatchFinderClassID = aMatchFinderInfo.ClassID;
    }
    aDefined = true;
    
    #endif
    if (!aDefined)
      return E_FAIL;
    
    aMethodMode.Methods.Add(aMethodFull);
  }
  aMethodMode.m_Binds = m_Binds;
  if (m_CompressHeaders)
    aHeaderMethod.Methods.Add(aMethodMode.Methods.Back());
  return S_OK;
}

STDMETHODIMP CHandler::UpdateItems(IOutStream *anOutStream, UINT32 aNumItems,
    IUpdateCallBack *anUpdateCallBack)
{
  COM_TRY_BEGIN

  CRecordVector<bool> aCompressStatuses;
  CObjectVector<CUpdateItemInfo> anUpdateItems;
  CRecordVector<UINT32> aCopyIndexes;
  
  CComPtr<IUpdateCallBack2> anUpdateCallBack2;
  anUpdateCallBack->QueryInterface(&anUpdateCallBack2);

  int anIndex = 0;
  for(int i = 0; i < aNumItems; i++)
  {
    CUpdateItemInfo anUpdateItemInfo;
    INT32 anCompress;
    INT32 anExistInArchive;
    INT32 anIndexInServer;
    CComBSTR aName;
    bool anIsAnti;
    if (anUpdateCallBack2)
    {
      INT32 _anIsAnti;
      RETURN_IF_NOT_S_OK(anUpdateCallBack2->GetUpdateItemInfo2(i,
        &anCompress, // 1 - compress 0 - copy
        &anExistInArchive,
        &anIndexInServer,
        &anUpdateItemInfo.Attributes,
        &anUpdateItemInfo.CreationTime,
        NULL,
        &anUpdateItemInfo.LastWriteTime,
        &anUpdateItemInfo.Size, 
        &aName,
        &_anIsAnti));
        anIsAnti = MyBoolToBool(_anIsAnti);
    }
    else
    {
      RETURN_IF_NOT_S_OK(anUpdateCallBack->GetUpdateItemInfo(i,
        &anCompress, // 1 - compress 0 - copy
        &anExistInArchive,
        &anIndexInServer,
        &anUpdateItemInfo.Attributes,
        &anUpdateItemInfo.CreationTime,
        NULL,
        &anUpdateItemInfo.LastWriteTime,
        &anUpdateItemInfo.Size, 
        &aName));
      anIsAnti = false;
    }
    if (MyBoolToBool(anCompress))
    {
      anUpdateItemInfo.IsAnti = anIsAnti;
      anUpdateItemInfo.SetDirectoryStatusFromAttributes();

      if (aName)
        anUpdateItemInfo.Name = aName;

      anUpdateItemInfo.AttributesAreDefined = true;
      anUpdateItemInfo.CreationTimeIsDefined = true;
      anUpdateItemInfo.LastWriteTimeIsDefined = true;

      anUpdateItemInfo.IndexInClient = i;

      if (anIsAnti)
      {
        anUpdateItemInfo.AttributesAreDefined = false;
        anUpdateItemInfo.CreationTimeIsDefined = false;
        anUpdateItemInfo.LastWriteTimeIsDefined = false;
        anUpdateItemInfo.Size = 0;
        if (MyBoolToBool(anExistInArchive) && !aName)
        {
          const CFileItemInfo &anItem = m_Database.m_Files[anIndexInServer];
          anUpdateItemInfo.Name = m_Database.m_Files[anIndexInServer].Name;
          anUpdateItemInfo.IsDirectory = anItem.IsDirectory;
        }
      }

      if(MyBoolToBool(anExistInArchive))
      {
        // const CFolderInfo &aFolderInfo = m_Folders[anIndexInServer];
        anUpdateItemInfo.Commented = false;
        if(anUpdateItemInfo.Commented)
        {
          // anUpdateItemInfo.CommentRange.Position = anItemInfo.GetCommentPosition();
          // anUpdateItemInfo.CommentRange.Size  = anItemInfo.CommentSize;
        }
      }
      else
        anUpdateItemInfo.Commented = false;
      aCompressStatuses.Add(true);
      anUpdateItems.Add(anUpdateItemInfo);
    }
    else
    {
      aCompressStatuses.Add(false);
      aCopyIndexes.Add(anIndexInServer);
    }
  }

  if (!aCopyIndexes.IsEmpty())
    for(int i = 0; i < m_Database.m_NumUnPackStreamsVector.Size(); i++)
      if (m_Database.m_NumUnPackStreamsVector[i] != 1)
        return E_FAIL;

  CCompressionMethodMode aMethodMode, aHeaderMethod;
  RETURN_IF_NOT_S_OK(SetCompressionMethod(aMethodMode, aHeaderMethod));

  NArchive::N7z::CInArchiveInfo *anInArchiveInfo;
  if (!m_InStream)
    anInArchiveInfo = 0;
  else
    anInArchiveInfo = &m_Database.m_ArchiveInfo;

  return UpdateMain(m_Database, aCompressStatuses,
      anUpdateItems, aCopyIndexes, anOutStream, m_InStream, anInArchiveInfo, 
      &aMethodMode, m_CompressHeaders ? &aHeaderMethod: 0, 
      anUpdateCallBack, m_Solid);
  COM_TRY_END
}

static const kMaxNumberOfDigitsInInputNumber = 9;

static int ParseNumberString(const AString &aString, int &aNumber)
{
  AString aNumberString;
  int i = 0;
  for(; i < aString.Length() && i < kMaxNumberOfDigitsInInputNumber; i++)
  {
    char aChar = aString[i];
    if(!isdigit(aChar) && (aChar != '-' || i > 0))
      break;
    aNumberString += aChar;
  }
  if (i > 0)
    aNumber = atoi(aNumberString);
  return i;
}

static UINT32 ParseUINT32String(const AString &aString, UINT32 &aNumber)
{
  int aNumberTemp;
  int aPos = ParseNumberString(aString, aNumberTemp);
  if (aPos <= 0)
    return aPos;
  if (aNumberTemp < 0)
    return 0;
  aNumber = aNumberTemp;
  return aPos;
}

static const kMaxLogarithmicSize = 31;

static const char kByteSymbol = 'B';
static const char kKiloByteSymbol = 'K';
static const char kMegaByteSymbol = 'M';

HRESULT ParseDictionaryValues(const AString &_aString, 
    BYTE &aLogDicSize, UINT32 &aDicSize)
{
  AString aString = _aString;
  int aNumber;
  aString.MakeUpper();
  int aNumDigits = ParseNumberString(aString, aNumber);
  if (aNumDigits == 0 || aString.Length() > aNumDigits + 1)
    return E_FAIL;
  if (aString.Length() == aNumDigits)
  {
    if (aNumber > kMaxLogarithmicSize)
      return E_FAIL;
    aLogDicSize = aNumber;
    aDicSize = 1 << aNumber;
    return S_OK;
  }
  switch (aString[aNumDigits])
  {
  case kByteSymbol:
    if (aNumber > (UINT32(1) << kMaxLogarithmicSize))
      return E_FAIL;
    aDicSize = aNumber;
    break;
  case kKiloByteSymbol:
    if (aNumber > (1 << (kMaxLogarithmicSize - 10)))
      return E_FAIL;
    aDicSize = aNumber << 10;
    break;
  case kMegaByteSymbol:
    if (aNumber > (1 << (kMaxLogarithmicSize - 20)))
      return E_FAIL;
    aDicSize = aNumber << 20;
    break;
  default:
    return E_FAIL;
  }
  for (int i = 0; i <= kMaxLogarithmicSize; i++)
    if (aDicSize <= (1 << i))
      break;
  aLogDicSize = i;
  return S_OK;
}

static inline UINT GetCurrentFileCodePage()
{
  return AreFileApisANSI() ? CP_ACP : CP_OEMCP;
}

static HRESULT SetBoolProperty(bool &aDest, const PROPVARIANT &aValue)
{
  switch(aValue.vt)
  {
    case VT_EMPTY:
      aDest = true;
      break;
    /*
    case VT_UI4:
      aDest = (aValue.ulVal != 0);
      break;
    */
    case VT_BSTR:
    {
      UString aValueString = aValue.bstrVal;
      aValueString.MakeUpper();
      if (aValueString.Compare(L"ON") == 0)
        aDest = true;
      else if (aValueString.Compare(L"OFF") == 0)
        aDest = false;
      else
        return E_INVALIDARG;
      break;
    }
    default:
      return E_INVALIDARG;
  }
  return S_OK;
}

static HRESULT GetBindInfoPart(AString &aString, UINT32 &aCoder, UINT32 &aStream)
{
  aStream = 0;
  int anIndex = ParseUINT32String(aString, aCoder);
  if (anIndex == 0)
    return E_INVALIDARG;
  aString.Delete(0, anIndex);
  if (aString[0] == 'S')
  {
    aString.Delete(0);
    int anIndex = ParseUINT32String(aString, aStream);
    if (anIndex == 0)
      return E_INVALIDARG;
    aString.Delete(0, anIndex);
  }
  return S_OK;
}

static HRESULT GetBindInfo(AString &aString, CBind &aBind)
{
  RETURN_IF_NOT_S_OK(GetBindInfoPart(aString, aBind.OutCoder, aBind.OutStream));
  if (aString[0] != ':')
    return E_INVALIDARG;
  aString.Delete(0);
  RETURN_IF_NOT_S_OK(GetBindInfoPart(aString, aBind.InCoder, aBind.InStream));
  if (!aString.IsEmpty())
    return E_INVALIDARG;
  return S_OK;
}

STDMETHODIMP CHandler::SetProperties(const BSTR *aNames, const PROPVARIANT *aValues, INT32 aNumProperties)
{
  UINT aCodePage = GetCurrentFileCodePage();
  COM_TRY_BEGIN
  m_Methods.Clear();
  m_Binds.Clear();
  Init();
  int aMinNumber = 0;

  for (int i = 0; i < aNumProperties; i++)
  {
    AString aName = UnicodeStringToMultiByte(UString(aNames[i]));
    aName.MakeUpper();

    const PROPVARIANT &aValue = aValues[i];

    if (aName.CompareNoCase("0") == 0 || 
        aName.CompareNoCase("1") == 0 || 
        aName.CompareNoCase("X") == 0)
    {
      if (aValue.vt == VT_EMPTY)
      {
        if (aName.CompareNoCase("X") == 0)
          m_DefaultDicSize = kDicSizeForX;
        continue;
      }
    }

    if (aName.IsEmpty())
      return E_INVALIDARG;
    if (aName[0] == 'B')
    {
      aName.Delete(0);
      CBind aBind;
      RETURN_IF_NOT_S_OK(GetBindInfo(aName, aBind));
      m_Binds.Add(aBind);
      continue;
    }

      
    int aNumber;
    int anIndex = ParseNumberString(aName, aNumber);
    AString aRealName = aName.Mid(anIndex);
    if (anIndex == 0)
    {
      if (aName.CompareNoCase("S") == 0)
      {
        RETURN_IF_NOT_S_OK(SetBoolProperty(m_Solid, aValue));
        continue;
      }
      else if (aName.CompareNoCase("HC") == 0)
      {
        RETURN_IF_NOT_S_OK(SetBoolProperty(m_CompressHeaders, aValue));
        continue;
      }
      aNumber = 0;
    }
    if (aNumber > 100)
      return E_FAIL;
    if (aNumber < aMinNumber)
    {
      /*
      for (int i = aNumber; i < aMinNumber; i++)
      {
        COneMethodInfo anOneMethodInfo;
        anOneMethodInfo.MatchFinderIsDefined = false;
        m_Methods.Insert(0, anOneMethodInfo);
      }
      aMinNumber = aNumber;
      */
      return E_INVALIDARG;
    }
    aNumber -= aMinNumber;
    for(int j = m_Methods.Size(); j <= aNumber; j++)
    {
      COneMethodInfo anOneMethodInfo;
      anOneMethodInfo.MatchFinderIsDefined = false;
      m_Methods.Add(anOneMethodInfo);
    }

    COneMethodInfo &anOneMethodInfo = m_Methods[aNumber];

    if (/*aRealName.CompareNoCase("M") == 0 || */ aRealName.Length() == 0)
    {
      if (aValue.vt != VT_BSTR)
        return E_INVALIDARG;
      anOneMethodInfo.MethodName = UnicodeStringToMultiByte(UString(aValue.bstrVal));
    }
    else if (aRealName.CompareNoCase("MF") == 0)
    {
      // if (aValue.vt != VT_UI4)
      if (aValue.vt != VT_BSTR)
        return E_INVALIDARG;
      anOneMethodInfo.MatchFinderIsDefined = true;
      // anOneMethodInfo.MatchFinderIndex = aValue.ulVal;
      anOneMethodInfo.MatchFinderName = GetSystemString(aValue.bstrVal);
    }
    else
    {
      CProperty aProperty;
      if (aRealName.CompareNoCase("D") == 0 || aRealName.CompareNoCase("MEM") == 0)
      {
        BYTE aLogDicSize;
        UINT32 aDicSize;
        if (aValue.vt == VT_UI4)
        {
          aLogDicSize = aValue.ulVal;
          aDicSize = 1 << aLogDicSize;
        }
        else if (aValue.vt == VT_BSTR)
        {
          RETURN_IF_NOT_S_OK(ParseDictionaryValues(UnicodeStringToMultiByte(aValue.bstrVal), 
              aLogDicSize, aDicSize));
        }
        else 
          return E_FAIL;
        if (aRealName.CompareNoCase("D") == 0)
          aProperty.PropID = NEncodedStreamProperies::kDictionarySize;
        else
          aProperty.PropID = NEncodedStreamProperies::kUsedMemorySize;
        aProperty.Value = aDicSize;
        anOneMethodInfo.CoderProperties.Add(aProperty);
      }
      else
      {
        int anIndex = FindPropIdFromStringName(aRealName);
        if (anIndex < 0)
          return E_INVALIDARG;
        
        const CNameToPropID &aNameToPropID = g_NameToPropID[anIndex];
        aProperty.PropID = aNameToPropID.PropID;
        
        if (!ConvertProperty(aValue, aNameToPropID.VarType, aProperty.Value))
          return E_INVALIDARG;
        
        if (aNameToPropID.CoderProperties)
          anOneMethodInfo.CoderProperties.Add(aProperty);
        else
          anOneMethodInfo.EncoderProperties.Add(aProperty);
      }
    }
  }

  return S_OK;
  COM_TRY_END
}  

