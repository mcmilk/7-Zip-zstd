// LoadCodecs.cpp

#include "StdAfx.h"

#include "LoadCodecs.h"

#include "../../../Common/MyCom.h"
#ifdef NEW_FOLDER_INTERFACE
#include "../../../Common/StringToInt.h"
#endif
#include "../../../Windows/PropVariant.h"

#include "../../ICoder.h"
#include "../../Common/RegisterArc.h"

#ifdef EXTERNAL_CODECS
#include "../../../Windows/FileFind.h"
#include "../../../Windows/DLL.h"
#ifdef NEW_FOLDER_INTERFACE
#include "../../../Windows/ResourceString.h"
static const UINT kIconTypesResId = 100;
#endif

#ifdef _WIN32
#include "Windows/Registry.h"
#endif

using namespace NWindows;
using namespace NFile;

#ifdef _WIN32
extern HINSTANCE g_hInstance;
#endif

static CSysString GetLibraryFolderPrefix()
{
  #ifdef _WIN32
  TCHAR fullPath[MAX_PATH + 1];
  ::GetModuleFileName(g_hInstance, fullPath, MAX_PATH);
  CSysString path = fullPath;
  int pos = path.ReverseFind(TEXT(CHAR_PATH_SEPARATOR));
  return path.Left(pos + 1);
  #else
  return CSysString(); // FIX IT
  #endif
}

#define kCodecsFolderName TEXT("Codecs")
#define kFormatsFolderName TEXT("Formats")
static TCHAR *kMainDll = TEXT("7z.dll");

#ifdef _WIN32
static LPCTSTR kRegistryPath = TEXT("Software\\7-zip");
static LPCTSTR kProgramPathValue = TEXT("Path");
static bool ReadPathFromRegistry(HKEY baseKey, CSysString &path)
{
  NRegistry::CKey key;
  if(key.Open(baseKey, kRegistryPath, KEY_READ) == ERROR_SUCCESS)
    if (key.QueryValue(kProgramPathValue, path) == ERROR_SUCCESS)
    {
      NName::NormalizeDirPathPrefix(path);
      return true;
    }
  return false;
}

#endif

CSysString GetBaseFolderPrefixFromRegistry()
{
  CSysString moduleFolderPrefix = GetLibraryFolderPrefix();
  NFind::CFileInfo fileInfo;
  if (NFind::FindFile(moduleFolderPrefix + kMainDll, fileInfo))
    if (!fileInfo.IsDirectory())
      return moduleFolderPrefix;
  if (NFind::FindFile(moduleFolderPrefix + kCodecsFolderName, fileInfo))
    if (fileInfo.IsDirectory())
      return moduleFolderPrefix;
  if (NFind::FindFile(moduleFolderPrefix + kFormatsFolderName, fileInfo))
    if (fileInfo.IsDirectory())
      return moduleFolderPrefix;
  #ifdef _WIN32
  CSysString path;
  if (ReadPathFromRegistry(HKEY_CURRENT_USER, path))
    return path;
  if (ReadPathFromRegistry(HKEY_LOCAL_MACHINE, path))
    return path;
  #endif
  return moduleFolderPrefix;
}

typedef UInt32 (WINAPI *GetNumberOfMethodsFunc)(UInt32 *numMethods);
typedef UInt32 (WINAPI *GetNumberOfFormatsFunc)(UInt32 *numFormats);
typedef UInt32 (WINAPI *GetHandlerPropertyFunc)(PROPID propID, PROPVARIANT *value);
typedef UInt32 (WINAPI *GetHandlerPropertyFunc2)(UInt32 index, PROPID propID, PROPVARIANT *value);
typedef UInt32 (WINAPI *CreateObjectFunc)(const GUID *clsID, const GUID *iid, void **outObject);
typedef UInt32 (WINAPI *SetLargePageModeFunc)();


static HRESULT GetCoderClass(GetMethodPropertyFunc getMethodProperty, UInt32 index, 
    PROPID propId, CLSID &clsId, bool &isAssigned)
{
  NWindows::NCOM::CPropVariant prop;
  isAssigned = false;
  RINOK(getMethodProperty(index, propId, &prop));
  if (prop.vt == VT_BSTR)
  {
    isAssigned = true;
    clsId = *(const GUID *)prop.bstrVal;
  }
  else if (prop.vt != VT_EMPTY)
    return E_FAIL;
  return S_OK;
}

HRESULT CCodecs::LoadCodecs()
{
  CCodecLib &lib = Libs.Back();
  lib.GetMethodProperty = (GetMethodPropertyFunc)lib.Lib.GetProcAddress("GetMethodProperty");
  if (lib.GetMethodProperty == NULL)
    return S_OK;

  UInt32 numMethods = 1;
  GetNumberOfMethodsFunc getNumberOfMethodsFunc = (GetNumberOfMethodsFunc)lib.Lib.GetProcAddress("GetNumberOfMethods");
  if (getNumberOfMethodsFunc != NULL)
  {
    RINOK(getNumberOfMethodsFunc(&numMethods));
  }

  for(UInt32 i = 0; i < numMethods; i++)
  {
    CDllCodecInfo info;
    info.LibIndex = Libs.Size() - 1;
    info.CodecIndex = i;

    RINOK(GetCoderClass(lib.GetMethodProperty, i, NMethodPropID::kEncoder, info.Encoder, info.EncoderIsAssigned));
    RINOK(GetCoderClass(lib.GetMethodProperty, i, NMethodPropID::kDecoder, info.Decoder, info.DecoderIsAssigned));

    Codecs.Add(info);
  }
  return S_OK;
}

static HRESULT ReadProp(
    GetHandlerPropertyFunc getProp, 
    GetHandlerPropertyFunc2 getProp2, 
    UInt32 index, PROPID propID, NCOM::CPropVariant &prop)
{
  if (getProp2)
    return getProp2(index, propID, &prop);;
  return getProp(propID, &prop);
}

static HRESULT ReadBoolProp(
    GetHandlerPropertyFunc getProp, 
    GetHandlerPropertyFunc2 getProp2, 
    UInt32 index, PROPID propID, bool &res)
{
  NCOM::CPropVariant prop;
  RINOK(ReadProp(getProp, getProp2, index, propID, prop));
  if (prop.vt == VT_BOOL)
    res = VARIANT_BOOLToBool(prop.boolVal);
  else if (prop.vt != VT_EMPTY)
    return E_FAIL;
  return S_OK;
}

static HRESULT ReadStringProp(
    GetHandlerPropertyFunc getProp, 
    GetHandlerPropertyFunc2 getProp2, 
    UInt32 index, PROPID propID, UString &res)
{
  NCOM::CPropVariant prop;
  RINOK(ReadProp(getProp, getProp2, index, propID, prop));
  if (prop.vt == VT_BSTR)
    res = prop.bstrVal;
  else if (prop.vt != VT_EMPTY)
    return E_FAIL;
  return S_OK;
}

#endif

static const unsigned int kNumArcsMax = 32;
static unsigned int g_NumArcs = 0;
static const CArcInfo *g_Arcs[kNumArcsMax]; 
void RegisterArc(const CArcInfo *arcInfo) 
{ 
  if (g_NumArcs < kNumArcsMax)
    g_Arcs[g_NumArcs++] = arcInfo; 
}

static void SplitString(const UString &srcString, UStringVector &destStrings)
{
  destStrings.Clear();
  UString s;
  int len = srcString.Length();
  if (len == 0)
    return;
  for (int i = 0; i < len; i++)
  {
    wchar_t c = srcString[i];
    if (c == L' ')
    {
      if (!s.IsEmpty())
      {
        destStrings.Add(s);
        s.Empty();
      }
    }
    else
      s += c;
  }
  if (!s.IsEmpty())
    destStrings.Add(s);
}

void CArcInfoEx::AddExts(const wchar_t* ext, const wchar_t* addExt)
{
  UStringVector exts, addExts;
  SplitString(ext, exts);
  if (addExt != 0)
    SplitString(addExt, addExts);
  for (int i = 0; i < exts.Size(); i++)
  {
    CArcExtInfo extInfo;
    extInfo.Ext = exts[i];
    if (i < addExts.Size())
    {
      extInfo.AddExt = addExts[i];
      if (extInfo.AddExt == L"*")
        extInfo.AddExt.Empty();
    }
    Exts.Add(extInfo);
  }
}

#ifdef EXTERNAL_CODECS

HRESULT CCodecs::LoadFormats()
{
  const NDLL::CLibrary &lib = Libs.Back().Lib;
  GetHandlerPropertyFunc getProp = 0;
  GetHandlerPropertyFunc2 getProp2 = (GetHandlerPropertyFunc2)
      lib.GetProcAddress("GetHandlerProperty2");
  if (getProp2 == NULL)
  {
    getProp = (GetHandlerPropertyFunc)
        lib.GetProcAddress("GetHandlerProperty");
    if (getProp == NULL)
      return S_OK;
  }

  UInt32 numFormats = 1;
  GetNumberOfFormatsFunc getNumberOfFormats = (GetNumberOfFormatsFunc)
    lib.GetProcAddress("GetNumberOfFormats");
  if (getNumberOfFormats != NULL)
  {
    RINOK(getNumberOfFormats(&numFormats));
  }
  if (getProp2 == NULL)
    numFormats = 1;

  for(UInt32 i = 0; i < numFormats; i++)
  {
    CArcInfoEx item;
    item.LibIndex = Libs.Size() - 1;
    item.FormatIndex = i;

    RINOK(ReadStringProp(getProp, getProp2, i, NArchive::kName, item.Name));

    NCOM::CPropVariant prop;
    if (ReadProp(getProp, getProp2, i, NArchive::kClassID, prop) != S_OK)
      continue;
    if (prop.vt != VT_BSTR)
      continue;
    item.ClassID = *(const GUID *)prop.bstrVal;
    prop.Clear();

    UString ext, addExt;
    RINOK(ReadStringProp(getProp, getProp2, i, NArchive::kExtension, ext));
    RINOK(ReadStringProp(getProp, getProp2, i, NArchive::kAddExtension, addExt));
    item.AddExts(ext, addExt);

    ReadBoolProp(getProp, getProp2, i, NArchive::kUpdate, item.UpdateEnabled);
    if (item.UpdateEnabled)
      ReadBoolProp(getProp, getProp2, i, NArchive::kKeepName, item.KeepName);
    
    if (ReadProp(getProp, getProp2, i, NArchive::kStartSignature, prop) == S_OK)
      if (prop.vt == VT_BSTR)
      {
        UINT len = ::SysStringByteLen(prop.bstrVal);
        item.StartSignature.SetCapacity(len);
        memmove(item.StartSignature, prop.bstrVal, len);
      }
    Formats.Add(item);
  }
  return S_OK;
}

#ifdef NEW_FOLDER_INTERFACE
void CCodecLib::LoadIcons()
{
  UString iconTypes = MyLoadStringW((HMODULE)Lib, kIconTypesResId);
  UStringVector pairs;
  SplitString(iconTypes, pairs);
  for (int i = 0; i < pairs.Size(); i++)
  {
    const UString &s = pairs[i];
    int pos = s.Find(L':');
    if (pos < 0)
      continue;
    CIconPair iconPair;
    const wchar_t *end;
    UString num = s.Mid(pos + 1);
    iconPair.IconIndex = (UInt32)ConvertStringToUInt64(num, &end);
    if (*end != L'\0')
      continue;
    iconPair.Ext = s.Left(pos);
    IconPairs.Add(iconPair);
  }
}

int CCodecLib::FindIconIndex(const UString &ext) const
{
  for (int i = 0; i < IconPairs.Size(); i++)
  {
    const CIconPair &pair = IconPairs[i];
    if (ext.CompareNoCase(pair.Ext) == 0)
      return pair.IconIndex;
  }
  return -1;
}
#endif

#ifdef _7ZIP_LARGE_PAGES
extern "C" 
{
  extern SIZE_T g_LargePageSize;
}
#endif

HRESULT CCodecs::LoadDll(const CSysString &dllPath)
{
  {
    NDLL::CLibrary library;
    if (!library.LoadEx(dllPath, LOAD_LIBRARY_AS_DATAFILE))
      return S_OK;
  }
  Libs.Add(CCodecLib());
  CCodecLib &lib = Libs.Back();
  #ifdef NEW_FOLDER_INTERFACE
  lib.Path = dllPath;
  #endif
  bool used = false;
  HRESULT res = S_OK;
  if (lib.Lib.Load(dllPath))
  {
    #ifdef NEW_FOLDER_INTERFACE
    lib.LoadIcons();
    #endif

    #ifdef _7ZIP_LARGE_PAGES
    if (g_LargePageSize != 0)
    {
      SetLargePageModeFunc setLargePageMode = (SetLargePageModeFunc)lib.Lib.GetProcAddress("SetLargePageMode");
      if (setLargePageMode != 0)
        setLargePageMode();
    }
    #endif

    lib.CreateObject = (CreateObjectFunc)lib.Lib.GetProcAddress("CreateObject");
    if (lib.CreateObject != 0)
    {
      int startSize = Codecs.Size();
      res = LoadCodecs();
      used = (Codecs.Size() != startSize);
      if (res == S_OK)
      {
        startSize = Formats.Size();
        res = LoadFormats();
        used = used || (Formats.Size() != startSize);
      }
    }
  }
  if (!used)
    Libs.DeleteBack();
  return res;
}

HRESULT CCodecs::LoadDllsFromFolder(const CSysString &folderPrefix)
{
  NFile::NFind::CEnumerator enumerator(folderPrefix + CSysString(TEXT("*")));
  NFile::NFind::CFileInfo fileInfo;
  while (enumerator.Next(fileInfo))
  {
    if (fileInfo.IsDirectory())
      continue;
    RINOK(LoadDll(folderPrefix + fileInfo.Name));
  }
  return S_OK;
}

#endif

#ifndef _SFX
static inline void SetBuffer(CByteBuffer &bb, const Byte *data, int size)
{
  bb.SetCapacity(size);
  memmove((Byte *)bb, data, size);
}
#endif

HRESULT CCodecs::Load()
{
  Formats.Clear();
  #ifdef EXTERNAL_CODECS
  Codecs.Clear();
  #endif
  for (UInt32 i = 0; i < g_NumArcs; i++)
  {
    const CArcInfo &arc = *g_Arcs[i];
    CArcInfoEx item;
    item.Name = arc.Name;
    item.CreateInArchive = arc.CreateInArchive;
    item.CreateOutArchive = arc.CreateOutArchive;
    item.AddExts(arc.Ext, arc.AddExt);
    item.UpdateEnabled = (arc.CreateOutArchive != 0);
    item.KeepName = arc.KeepName;

    #ifndef _SFX
    SetBuffer(item.StartSignature, arc.Signature, arc.SignatureSize);
    #endif
    Formats.Add(item);
  }
  #ifdef EXTERNAL_CODECS
  const CSysString baseFolder = GetBaseFolderPrefixFromRegistry();
  RINOK(LoadDll(baseFolder + kMainDll));
  RINOK(LoadDllsFromFolder(baseFolder + kCodecsFolderName TEXT(STRING_PATH_SEPARATOR)));
  RINOK(LoadDllsFromFolder(baseFolder + kFormatsFolderName TEXT(STRING_PATH_SEPARATOR)));
  #endif
  return S_OK;
}

int CCodecs::FindFormatForArchiveName(const UString &archivePath) const
{
  int slashPos1 = archivePath.ReverseFind(L'\\');
  int slashPos2 = archivePath.ReverseFind(L'.');
  int dotPos = archivePath.ReverseFind(L'.');
  if (dotPos < 0 || dotPos < slashPos1 || dotPos < slashPos2)
    return -1;
  UString ext = archivePath.Mid(dotPos + 1);
  for (int i = 0; i < Formats.Size(); i++)
  {
    const CArcInfoEx &arc = Formats[i];
    if (!arc.UpdateEnabled)
      continue;
    // if (arc.FindExtension(ext) >= 0)
    UString mainExt = arc.GetMainExt();
    if (!mainExt.IsEmpty() && ext.CompareNoCase(mainExt) == 0)
      return i;
  }
  return -1;
}

int CCodecs::FindFormatForArchiveType(const UString &arcType) const
{
  for (int i = 0; i < Formats.Size(); i++)
  {
    const CArcInfoEx &arc = Formats[i];
    if (!arc.UpdateEnabled)
      continue;
    if (arc.Name.CompareNoCase(arcType) == 0)
      return i;
  }
  return -1;
}

#ifdef EXTERNAL_CODECS

#ifdef EXPORT_CODECS
extern unsigned int g_NumCodecs;
STDAPI CreateCoder2(bool encode, UInt32 index, const GUID *iid, void **outObject);
STDAPI GetMethodProperty(UInt32 codecIndex, PROPID propID, PROPVARIANT *value);
// STDAPI GetNumberOfMethods(UINT32 *numCodecs);
#endif

STDMETHODIMP CCodecs::GetNumberOfMethods(UINT32 *numMethods)
{
  *numMethods = 
      #ifdef EXPORT_CODECS
      g_NumCodecs + 
      #endif
      Codecs.Size();
  return S_OK;
}

STDMETHODIMP CCodecs::GetProperty(UINT32 index, PROPID propID, PROPVARIANT *value)
{
  #ifdef EXPORT_CODECS
  if (index < g_NumCodecs)
    return GetMethodProperty(index, propID, value);
  #endif

  const CDllCodecInfo &ci = Codecs[index 
      #ifdef EXPORT_CODECS
      - g_NumCodecs
      #endif
      ];

  if (propID == NMethodPropID::kDecoderIsAssigned)
  {
    NWindows::NCOM::CPropVariant propVariant;
    propVariant = ci.DecoderIsAssigned;
    propVariant.Detach(value);
    return S_OK;
  }
  if (propID == NMethodPropID::kEncoderIsAssigned)
  {
    NWindows::NCOM::CPropVariant propVariant;
    propVariant = ci.EncoderIsAssigned;
    propVariant.Detach(value);
    return S_OK;
  }
  return Libs[ci.LibIndex].GetMethodProperty(ci.CodecIndex, propID, value);
}

STDMETHODIMP CCodecs::CreateDecoder(UINT32 index, const GUID *iid, void **coder)
{
  #ifdef EXPORT_CODECS
  if (index < g_NumCodecs)
    return CreateCoder2(false, index, iid, coder);
  #endif
  const CDllCodecInfo &ci = Codecs[index 
      #ifdef EXPORT_CODECS
      - g_NumCodecs
      #endif
      ];
  if (ci.DecoderIsAssigned)
    return Libs[ci.LibIndex].CreateObject(&ci.Decoder, iid, (void **)coder);
  return S_OK;
}

STDMETHODIMP CCodecs::CreateEncoder(UINT32 index, const GUID *iid, void **coder)
{
  #ifdef EXPORT_CODECS
  if (index < g_NumCodecs)
    return CreateCoder2(true, index, iid, coder);
  #endif
  const CDllCodecInfo &ci = Codecs[index 
      #ifdef EXPORT_CODECS
      - g_NumCodecs
      #endif
      ];
  if (ci.EncoderIsAssigned)
    return Libs[ci.LibIndex].CreateObject(&ci.Encoder, iid, (void **)coder);
  return S_OK;
}

HRESULT CCodecs::CreateCoder(const UString &name, bool encode, CMyComPtr<ICompressCoder> &coder) const
{ 
  for (int i = 0; i < Codecs.Size(); i++)
  {
    const CDllCodecInfo &codec = Codecs[i];
    if (encode && !codec.EncoderIsAssigned || !encode && !codec.DecoderIsAssigned)
      continue;
    const CCodecLib &lib = Libs[codec.LibIndex];
    UString res;
    NWindows::NCOM::CPropVariant prop;
    RINOK(lib.GetMethodProperty(codec.CodecIndex, NMethodPropID::kName, &prop));
    if (prop.vt == VT_BSTR)
      res = prop.bstrVal;
    else if (prop.vt != VT_EMPTY)
      continue;
    if (name.CompareNoCase(res) == 0)
      return lib.CreateObject(encode ? &codec.Encoder : &codec.Decoder, &IID_ICompressCoder, (void **)&coder);
  }
  return CLASS_E_CLASSNOTAVAILABLE;
}

int CCodecs::GetCodecLibIndex(UInt32 index)
{
  #ifdef EXPORT_CODECS
  if (index < g_NumCodecs)
    return -1;
  #endif
  #ifdef EXTERNAL_CODECS
  const CDllCodecInfo &ci = Codecs[index 
      #ifdef EXPORT_CODECS
      - g_NumCodecs
      #endif
      ];
  return ci.LibIndex;
  #else
  return -1;
  #endif
}

bool CCodecs::GetCodecEncoderIsAssigned(UInt32 index)
{
  #ifdef EXPORT_CODECS
  if (index < g_NumCodecs)
  {
    NWindows::NCOM::CPropVariant prop;
    if (GetProperty(index, NMethodPropID::kEncoder, &prop) == S_OK)
      if (prop.vt != VT_EMPTY)
        return true;
    return false;
  }
  #endif
  #ifdef EXTERNAL_CODECS
  const CDllCodecInfo &ci = Codecs[index 
      #ifdef EXPORT_CODECS
      - g_NumCodecs
      #endif
      ];
  return ci.EncoderIsAssigned;
  #else
  return false;
  #endif
}

HRESULT CCodecs::GetCodecId(UInt32 index, UInt64 &id)
{
  UString s;
  NWindows::NCOM::CPropVariant prop;
  RINOK(GetProperty(index, NMethodPropID::kID, &prop));
  if (prop.vt != VT_UI8)
    return E_INVALIDARG;
  id = prop.uhVal.QuadPart;
  return S_OK;
}

UString CCodecs::GetCodecName(UInt32 index)
{
  UString s;
  NWindows::NCOM::CPropVariant prop;
  if (GetProperty(index, NMethodPropID::kName, &prop) == S_OK)
    if (prop.vt == VT_BSTR)
      s = prop.bstrVal;
  return s;
}

#endif
