// ExtractSTD.h

#pragma once

#ifndef __EXTRACTSTD_H
#define __EXTRACTSTD_H

#include "Common/Wildcard.h"
#include "Windows/FileFind.h"

#include "../Common/IArchiveHandler2.h"
#include "../Common/ZipSettings.h"

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

  NZipSettings::NExtraction::NOverwriteMode::EEnum OverwriteMode;

  
  CExtractOptions(NExtractMode::EEnum anExtractMode, const CSysString &anOutputBaseDir,
      bool anYesToAll, bool aPasswordEnabled, const UString &aPassword,
      NZipSettings::NExtraction::NOverwriteMode::EEnum anOverwriteMode):
    ExtractMode(anExtractMode),
    OutputBaseDir(anOutputBaseDir),
    YesToAll(anYesToAll),
    PasswordEnabled(aPasswordEnabled), 
    Password(aPassword),
    OverwriteMode(anOverwriteMode)
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
