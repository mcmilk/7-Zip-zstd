// List.h

#pragma once

#ifndef __LIST_H
#define __LIST_H

#include "../../Archive/IArchive.h"
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

HRESULT ListArchive(IInArchive *archive, 
    const UString &defaultItemName,
    const NWindows::NFile::NFind::CFileInfoW &srchiveFileInfo,
    const NWildcard::CCensor &wildcardCensor/*, bool fullPathMode, 
    NListMode::EEnum mode*/);

#endif

