// ListArchive.h

#pragma once

#ifndef __LISTARCHIVE_H
#define __LISTARCHIVE_H

#include "../Common/IArchiveHandler2.h"
#include "Common/Wildcard.h"
#include "Windows/FileFind.h"

/*
namespace NListMode
{
  enum EEnum
  {
    kDefault,
    kAdd,
    kAll
  };
}
*/

HRESULT ListArchive(IArchiveHandler200 *anArchive, 
    const UString &aDefaultItemName,
    const NWindows::NFile::NFind::CFileInfo &anArchiveFileInfo,
    const NWildcard::CCensor &aWildcardCensor/*, bool aFullPathMode, 
    NListMode::EEnum aMode*/);

#endif

