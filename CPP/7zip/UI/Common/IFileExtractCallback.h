// IFileExtractCallback.h

#ifndef __IFILEEXTRACTCALLBACK_H
#define __IFILEEXTRACTCALLBACK_H

#include "Common/String.h"

namespace NOverwriteAnswer
{
  enum EEnum
  {
    kYes,
    kYesToAll,
    kNo,
    kNoToAll,
    kAutoRename,
    kCancel,
  };
}

// {23170F69-40C1-278A-0000-000100070000}
DEFINE_GUID(IID_IFolderArchiveExtractCallback, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x07, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100070000")
IFolderArchiveExtractCallback: public IProgress
{
public:
  STDMETHOD(AskOverwrite)(
      const wchar_t *existName, const FILETIME *existTime, const UInt64 *existSize,
      const wchar_t *newName, const FILETIME *newTime, const UInt64 *newSize,
      Int32 *answer) PURE;
  STDMETHOD(PrepareOperation)(const wchar_t *name, Int32 askExtractMode, const UInt64 *position) PURE;
  STDMETHOD(MessageError)(const wchar_t *message) PURE;
  STDMETHOD(SetOperationResult)(Int32 operationResult, bool encrypted) PURE;
};

struct IExtractCallbackUI: IFolderArchiveExtractCallback
{
  virtual HRESULT BeforeOpen(const wchar_t *name) = 0;
  virtual HRESULT OpenResult(const wchar_t *name, HRESULT result, bool encrypted) = 0;
  virtual HRESULT ThereAreNoFiles() = 0;
  virtual HRESULT ExtractResult(HRESULT result) = 0;
  virtual HRESULT SetPassword(const UString &password) = 0;
};

#endif
