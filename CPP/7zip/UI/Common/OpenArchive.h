// OpenArchive.h

#ifndef __OPENARCHIVE_H
#define __OPENARCHIVE_H

#include "Common/MyString.h"
#include "Windows/FileFind.h"

#include "../../Archive/IArchive.h"
#include "LoadCodecs.h"
#include "ArchiveOpenCallback.h"

HRESULT GetArchiveItemPath(IInArchive *archive, UInt32 index, UString &result);
HRESULT GetArchiveItemPath(IInArchive *archive, UInt32 index, const UString &defaultName, UString &result);
HRESULT GetArchiveItemFileTime(IInArchive *archive, UInt32 index, 
    const FILETIME &defaultFileTime, FILETIME &fileTime);
HRESULT IsArchiveItemProp(IInArchive *archive, UInt32 index, PROPID propID, bool &result);
HRESULT IsArchiveItemFolder(IInArchive *archive, UInt32 index, bool &result);
HRESULT IsArchiveItemAnti(IInArchive *archive, UInt32 index, bool &result);

struct ISetSubArchiveName
{
  virtual void SetSubArchiveName(const wchar_t *name) = 0;
};

HRESULT OpenArchive(
    CCodecs *codecs,
    IInStream *inStream,
    const UString &fileName, 
    IInArchive **archiveResult, 
    int &formatIndex,
    UString &defaultItemName,
    IArchiveOpenCallback *openArchiveCallback);

HRESULT OpenArchive(
    CCodecs *codecs,
    const UString &filePath, 
    IInArchive **archive, 
    int &formatIndex,
    UString &defaultItemName,
    IArchiveOpenCallback *openArchiveCallback);

HRESULT OpenArchive(
    CCodecs *codecs,
    const UString &filePath, 
    IInArchive **archive0, 
    IInArchive **archive1, 
    int &formatIndex0,
    int &formatIndex1,
    UString &defaultItemName0,
    UString &defaultItemName1,
    IArchiveOpenCallback *openArchiveCallback);


HRESULT ReOpenArchive(IInArchive *archive, const UString &fileName, IArchiveOpenCallback *openArchiveCallback);

HRESULT MyOpenArchive(
    CCodecs *codecs,
    const UString &archiveName, 
    IInArchive **archive,
    UString &defaultItemName,
    IOpenCallbackUI *openCallbackUI);

HRESULT MyOpenArchive(
    CCodecs *codecs,
    const UString &archiveName, 
    IInArchive **archive0,
    IInArchive **archive1,
    UString &defaultItemName0,
    UString &defaultItemName1,
    UStringVector &volumePaths,
    UInt64 &volumesSize,
    IOpenCallbackUI *openCallbackUI);

struct CArchiveLink
{
  CMyComPtr<IInArchive> Archive0;
  CMyComPtr<IInArchive> Archive1;
  UString DefaultItemName0;
  UString DefaultItemName1;

  int FormatIndex0;
  int FormatIndex1;
  
  UStringVector VolumePaths;

  UInt64 VolumesSize;

  int GetNumLevels() const
  { 
    int result = 0;
    if (Archive0)
    {
      result++;
      if (Archive1)
        result++;
    }
    return result;
  }

  bool IsOpen;

  CArchiveLink(): IsOpen(false), VolumesSize(0) {};

  IInArchive *GetArchive() { return Archive1 != 0 ? Archive1: Archive0; }
  UString GetDefaultItemName()  { return Archive1 != 0 ? DefaultItemName1: DefaultItemName0; }
  int GetArchiverIndex() const { return Archive1 != 0 ? FormatIndex1: FormatIndex0; }
  HRESULT Close();
  void Release();
};

HRESULT OpenArchive(
    CCodecs *codecs,
    const UString &archiveName, 
    CArchiveLink &archiveLink,
    IArchiveOpenCallback *openCallback);

HRESULT MyOpenArchive(
    CCodecs *codecs,
    const UString &archiveName, 
    CArchiveLink &archiveLink,
    IOpenCallbackUI *openCallbackUI);

HRESULT ReOpenArchive(
    CCodecs *codecs,
    CArchiveLink &archiveLink, 
    const UString &fileName);

#endif

