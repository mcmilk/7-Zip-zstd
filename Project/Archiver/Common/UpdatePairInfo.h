// UpdatePairInfo.h

#pragma once

#ifndef __UPDATEPAIRINFO_H
#define __UPDATEPAIRINFO_H

// #include "Archive/Zip/ItemInfoEx.h"

#include "ArchiveStyleDirItemInfo.h"
#include "UpdatePairBasic.h"

#include "../Format/Common/ArchiveInterface.h"

struct CUpdatePairInfo
{
  NUpdateArchive::NPairState::EEnum State;
  int ArchiveItemIndex;
  int DirItemIndex;
};

typedef CObjectVector<CUpdatePairInfo> CUpdatePairInfoVector;

void GetUpdatePairInfoList(const CArchiveStyleDirItemInfoVector &dirItems, 
    const CArchiveItemInfoVector &archiveItems,
    NFileTimeType::EEnum fileTimeType,
    CUpdatePairInfoVector &updatePairs);

#endif