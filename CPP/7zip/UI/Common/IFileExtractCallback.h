// IFileExtractCallback.h

#ifndef __I_FILE_EXTRACT_CALLBACK_H
#define __I_FILE_EXTRACT_CALLBACK_H

#include "../../../Common/MyString.h"

#include "../../IDecl.h"

namespace NOverwriteAnswer
{
  enum EEnum
  {
    kYes,
    kYesToAll,
    kNo,
    kNoToAll,
    kAutoRename,
    kCancel
  };
}

DECL_INTERFACE_SUB(IFolderArchiveExtractCallback, IProgress, 0x01, 0x07)
{
public:
  STDMETHOD(AskOverwrite)(
      const wchar_t *existName, const FILETIME *existTime, const UInt64 *existSize,
      const wchar_t *newName, const FILETIME *newTime, const UInt64 *newSize,
      Int32 *answer) PURE;
  STDMETHOD(PrepareOperation)(const wchar_t *name, bool isFolder, Int32 askExtractMode, const UInt64 *position) PURE;
  STDMETHOD(MessageError)(const wchar_t *message) PURE;
  STDMETHOD(SetOperationResult)(Int32 operationResult, bool encrypted) PURE;
};

struct IExtractCallbackUI: IFolderArchiveExtractCallback
{
  virtual HRESULT BeforeOpen(const wchar_t *name) = 0;
  virtual HRESULT OpenResult(const wchar_t *name, HRESULT result, bool encrypted) = 0;
  virtual HRESULT SetError(int level, const wchar_t *name,
      UInt32 errorFlags, const wchar_t *errors,
      UInt32 warningFlags, const wchar_t *warnings) = 0;
  virtual HRESULT ThereAreNoFiles() = 0;
  virtual HRESULT ExtractResult(HRESULT result) = 0;
  virtual HRESULT OpenTypeWarning(const wchar_t *name, const wchar_t *okType, const wchar_t *errorType) = 0;

  #ifndef _NO_CRYPTO
  virtual HRESULT SetPassword(const UString &password) = 0;
  #endif
};


#define INTERFACE_IGetProp(x) \
  STDMETHOD(GetProp)(PROPID propID, PROPVARIANT *value) x; \

DECL_INTERFACE_SUB(IGetProp, IUnknown, 0x01, 0x20)
{
  INTERFACE_IGetProp(PURE)
};

#define INTERFACE_IFolderExtractToStreamCallback(x) \
  STDMETHOD(UseExtractToStream)(Int32 *res) x; \
  STDMETHOD(GetStream7)(const wchar_t *name, Int32 isDir, ISequentialOutStream **outStream, Int32 askExtractMode, IGetProp *getProp) x; \
  STDMETHOD(PrepareOperation7)(Int32 askExtractMode) x; \
  STDMETHOD(SetOperationResult7)(Int32 resultEOperationResult, bool encrypted) x; \

DECL_INTERFACE_SUB(IFolderExtractToStreamCallback, IUnknown, 0x01, 0x30)
{
  INTERFACE_IFolderExtractToStreamCallback(PURE)
};


#endif
