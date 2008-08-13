// PropertyName.cpp

#include "StdAfx.h"

#include "Common/IntToString.h"
#include "Windows/ResourceString.h"

#include "../../PropID.h"

#include "resource.h"
#include "PropertyNameRes.h"
#include "LangUtils.h"
#include "PropertyName.h"

struct CPropertyIDNamePair
{
  PROPID PropID;
  UINT ResourceID;
  UInt32 LangID;
};

static CPropertyIDNamePair kPropertyIDNamePairs[] =
{
  { kpidPath, IDS_PROPERTY_PATH, 0x02000203 },
  { kpidName, IDS_PROPERTY_NAME, 0x02000204 },
  { kpidExtension, IDS_PROPERTY_EXTENSION, 0x02000205 },
  { kpidIsDir, IDS_PROPERTY_IS_FOLDER,  0x02000206},
  { kpidSize, IDS_PROPERTY_SIZE, 0x02000207},
  { kpidPackSize, IDS_PROPERTY_PACKED_SIZE, 0x02000208 },
  { kpidAttrib, IDS_PROPERTY_ATTRIBUTES, 0x02000209 },
  { kpidCTime, IDS_PROPERTY_CTIME, 0x0200020A },
  { kpidATime, IDS_PROPERTY_ATIME, 0x0200020B },
  { kpidMTime, IDS_PROPERTY_MTIME, 0x0200020C },
  { kpidSolid, IDS_PROPERTY_SOLID, 0x0200020D },
  { kpidCommented, IDS_PROPERTY_C0MMENTED, 0x0200020E },
  { kpidEncrypted, IDS_PROPERTY_ENCRYPTED, 0x0200020F },
  { kpidSplitBefore, IDS_PROPERTY_SPLIT_BEFORE, 0x02000210 },
  { kpidSplitAfter, IDS_PROPERTY_SPLIT_AFTER, 0x02000211 },
  { kpidDictionarySize, IDS_PROPERTY_DICTIONARY_SIZE, 0x02000212 },
  { kpidCRC, IDS_PROPERTY_CRC, 0x02000213 },
  { kpidType, IDS_PROPERTY_FILE_TYPE,  0x02000214},
  { kpidIsAnti, IDS_PROPERTY_ANTI, 0x02000215 },
  { kpidMethod, IDS_PROPERTY_METHOD, 0x02000216 },
  { kpidHostOS, IDS_PROPERTY_HOST_OS, 0x02000217 },
  { kpidFileSystem, IDS_PROPERTY_FILE_SYSTEM, 0x02000218},
  { kpidUser, IDS_PROPERTY_USER, 0x02000219},
  { kpidGroup, IDS_PROPERTY_GROUP, 0x0200021A},
  { kpidBlock, IDS_PROPERTY_BLOCK, 0x0200021B },
  { kpidComment, IDS_PROPERTY_COMMENT, 0x0200021C },
  { kpidPosition, IDS_PROPERTY_POSITION, 0x0200021D },
  { kpidPrefix, IDS_PROPERTY_PREFIX, 0x0200021E },
  { kpidNumSubDirs, IDS_PROPERTY_FOLDERS, 0x0200021F },
  { kpidNumSubFiles, IDS_PROPERTY_FILES, 0x02000220 },
  { kpidUnpackVer, IDS_PROPERTY_VERSION, 0x02000221},
  { kpidVolume, IDS_PROPERTY_VOLUME, 0x02000222},
  { kpidIsVolume, IDS_PROPERTY_IS_VOLUME, 0x02000223},
  { kpidOffset, IDS_PROPERTY_OFFSET, 0x02000224},
  { kpidLinks, IDS_PROPERTY_LINKS, 0x02000225},
  { kpidNumBlocks, IDS_PROPERTY_NUM_BLOCKS, 0x02000226},
  { kpidNumVolumes, IDS_PROPERTY_NUM_VOLUMES, 0x02000227},

  { kpidBit64, IDS_PROPERTY_BIT64, 0x02000229},
  { kpidBigEndian, IDS_PROPERTY_BIG_ENDIAN, 0x0200022A},
  { kpidCpu, IDS_PROPERTY_CPU, 0x0200022B},
  { kpidPhySize, IDS_PROPERTY_PHY_SIZE, 0x0200022C},
  { kpidHeadersSize, IDS_PROPERTY_HEADERS_SIZE, 0x0200022D},
  { kpidChecksum, IDS_PROPERTY_CHECKSUM, 0x0200022E},
  { kpidCharacts, IDS_PROPERTY_CHARACTS, 0x0200022F},
  { kpidVa, IDS_PROPERTY_VA, 0x02000230},

  { kpidTotalSize, IDS_PROPERTY_TOTAL_SIZE, 0x03031100 },
  { kpidFreeSpace, IDS_PROPERTY_FREE_SPACE, 0x03031101 },
  { kpidClusterSize, IDS_PROPERTY_CLUSTER_SIZE, 0x03031102},
  { kpidVolumeName, IDS_PROPERTY_VOLUME_NAME, 0x03031103 },

  { kpidLocalName, IDS_PROPERTY_LOCAL_NAME, 0x03031200 },
  { kpidProvider, IDS_PROPERTY_PROVIDER, 0x03031201 }
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
    wchar_t s[32];
    ConvertUInt64ToString(propID, s);
    return s;
  }
  const CPropertyIDNamePair &pair = kPropertyIDNamePairs[index];
  return LangString(pair.ResourceID, pair.LangID);
}
