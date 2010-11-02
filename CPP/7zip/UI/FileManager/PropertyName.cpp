// PropertyName.cpp

#include "StdAfx.h"

#include "Common/IntToString.h"

#include "Windows/ResourceString.h"

#include "../../PropID.h"

#include "LangUtils.h"
#include "PropertyName.h"

#include "resource.h"
#include "PropertyNameRes.h"

struct CPropertyIDNamePair
{
  PROPID PropID;
  UINT ResourceID;
  UInt32 LangID;
};

static CPropertyIDNamePair kPropertyIDNamePairs[] =
{
  { kpidPath, IDS_PROP_PATH, 0x02000203 },
  { kpidName, IDS_PROP_NAME, 0x02000204 },
  { kpidExtension, IDS_PROP_EXTENSION, 0x02000205 },
  { kpidIsDir, IDS_PROP_IS_FOLDER, 0x02000206},
  { kpidSize, IDS_PROP_SIZE, 0x02000207},
  { kpidPackSize, IDS_PROP_PACKED_SIZE, 0x02000208 },
  { kpidAttrib, IDS_PROP_ATTRIBUTES, 0x02000209 },
  { kpidCTime, IDS_PROP_CTIME, 0x0200020A },
  { kpidATime, IDS_PROP_ATIME, 0x0200020B },
  { kpidMTime, IDS_PROP_MTIME, 0x0200020C },
  { kpidSolid, IDS_PROP_SOLID, 0x0200020D },
  { kpidCommented, IDS_PROP_C0MMENTED, 0x0200020E },
  { kpidEncrypted, IDS_PROP_ENCRYPTED, 0x0200020F },
  { kpidSplitBefore, IDS_PROP_SPLIT_BEFORE, 0x02000210 },
  { kpidSplitAfter, IDS_PROP_SPLIT_AFTER, 0x02000211 },
  { kpidDictionarySize, IDS_PROP_DICTIONARY_SIZE, 0x02000212 },
  { kpidCRC, IDS_PROP_CRC, 0x02000213 },
  { kpidType, IDS_PROP_FILE_TYPE, 0x02000214},
  { kpidIsAnti, IDS_PROP_ANTI, 0x02000215 },
  { kpidMethod, IDS_PROP_METHOD, 0x02000216 },
  { kpidHostOS, IDS_PROP_HOST_OS, 0x02000217 },
  { kpidFileSystem, IDS_PROP_FILE_SYSTEM, 0x02000218},
  { kpidUser, IDS_PROP_USER, 0x02000219},
  { kpidGroup, IDS_PROP_GROUP, 0x0200021A},
  { kpidBlock, IDS_PROP_BLOCK, 0x0200021B },
  { kpidComment, IDS_PROP_COMMENT, 0x0200021C },
  { kpidPosition, IDS_PROP_POSITION, 0x0200021D },
  { kpidPrefix, IDS_PROP_PREFIX, 0x0200021E },
  { kpidNumSubDirs, IDS_PROP_FOLDERS, 0x0200021F },
  { kpidNumSubFiles, IDS_PROP_FILES, 0x02000220 },
  { kpidUnpackVer, IDS_PROP_VERSION, 0x02000221},
  { kpidVolume, IDS_PROP_VOLUME, 0x02000222},
  { kpidIsVolume, IDS_PROP_IS_VOLUME, 0x02000223},
  { kpidOffset, IDS_PROP_OFFSET, 0x02000224},
  { kpidLinks, IDS_PROP_LINKS, 0x02000225},
  { kpidNumBlocks, IDS_PROP_NUM_BLOCKS, 0x02000226},
  { kpidNumVolumes, IDS_PROP_NUM_VOLUMES, 0x02000227},

  { kpidBit64, IDS_PROP_BIT64, 0x02000229},
  { kpidBigEndian, IDS_PROP_BIG_ENDIAN, 0x0200022A},
  { kpidCpu, IDS_PROP_CPU, 0x0200022B},
  { kpidPhySize, IDS_PROP_PHY_SIZE, 0x0200022C},
  { kpidHeadersSize, IDS_PROP_HEADERS_SIZE, 0x0200022D},
  { kpidChecksum, IDS_PROP_CHECKSUM, 0x0200022E},
  { kpidCharacts, IDS_PROP_CHARACTS, 0x0200022F},
  { kpidVa, IDS_PROP_VA, 0x02000230},
  { kpidId, IDS_PROP_ID, 0x02000231 },
  { kpidShortName, IDS_PROP_SHORT_NAME, 0x02000232 },
  { kpidCreatorApp, IDS_PROP_CREATOR_APP, 0x02000233 },
  { kpidSectorSize, IDS_PROP_SECTOR_SIZE, 0x02000234 },
  { kpidPosixAttrib, IDS_PROP_POSIX_ATTRIB, 0x02000235 },
  { kpidLink, IDS_PROP_LINK, 0x02000236 },
  { kpidError, IDS_PROP_ERROR, 0x02000605 },
 
  { kpidTotalSize, IDS_PROP_TOTAL_SIZE, 0x03031100 },
  { kpidFreeSpace, IDS_PROP_FREE_SPACE, 0x03031101 },
  { kpidClusterSize, IDS_PROP_CLUSTER_SIZE, 0x03031102},
  { kpidVolumeName, IDS_PROP_VOLUME_NAME, 0x03031103 },

  { kpidLocalName, IDS_PROP_LOCAL_NAME, 0x03031200 },
  { kpidProvider, IDS_PROP_PROVIDER, 0x03031201 }
};

int FindProperty(PROPID propID)
{
  for (int i = 0; i < sizeof(kPropertyIDNamePairs) / sizeof(kPropertyIDNamePairs[0]); i++)
    if (kPropertyIDNamePairs[i].PropID == propID)
      return i;
  return -1;
}

UString GetNameOfProperty(PROPID propID, const wchar_t *name)
{
  int index = FindProperty(propID);
  if (index < 0)
  {
    if (name)
      return name;
    wchar_t s[16];
    ConvertUInt32ToString(propID, s);
    return s;
  }
  const CPropertyIDNamePair &pair = kPropertyIDNamePairs[index];
  return LangString(pair.ResourceID, pair.LangID);
}
