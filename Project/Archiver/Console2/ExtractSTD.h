// ExtractSTD.h

#pragma once

#ifndef __EXTRACTSTD_H
#define __EXTRACTSTD_H

#include "Common/Wildcard.h"
#include "Windows/FileFind.h"

#include "../Common/IArchiveHandler2.h"

namespace NExtractMode {
enum EEnum
{
  kTest,
  kFullPath,
  kExtractToOne
};

}

class CExtractOptions
{
public:
  NExtractMode::EEnum ExtractMode;
  CSysString OutputBaseDir;
  bool YesToAll;
  UString DefaultItemName;
  NWindows::NFile::NFind::CFileInfo ArchiveFileInfo;
  bool PasswordEnabled;
  UString Password;

  
  CExtractOptions(NExtractMode::EEnum anExtractMode, const CSysString &anOutputBaseDir,
      bool anYesToAll, bool aPasswordEnabled, const UString &aPassword):
    ExtractMode(anExtractMode),
    OutputBaseDir(anOutputBaseDir),
    YesToAll(anYesToAll),
    PasswordEnabled(aPasswordEnabled), 
    Password(aPassword)
    {}

  bool TestMode() const {  return (ExtractMode == NExtractMode::kTest); }
  bool FullPathMode() const { return (ExtractMode == NExtractMode::kTest) || 
    (ExtractMode == NExtractMode::kFullPath); }
};

HRESULT DeCompressArchiveSTD(IArchiveHandler200 *anArchive,
    const NWildcard::CCensor &aWildcardCensor,
    const CExtractOptions &anOptions);

/*
bool DeCompressArchiveSTD(TTWildCardInputArchive &anArchive, 
    const TTExtractOptions &anOptions);
*/

#endif
