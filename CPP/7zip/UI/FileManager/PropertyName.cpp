// PropertyName.cpp

#include "StdAfx.h"

#include "../../PropID.h"
#include "Windows/ResourceString.h"
#include "resource.h"
#include "PropertyName.h"
#include "PropertyNameRes.h"
#include "LangUtils.h"

struct CPropertyIDNamePair
{
  PROPID PropID;
  UINT ResourceID;
  UINT LangID;
};

static CPropertyIDNamePair kPropertyIDNamePairs[] =  
{
  { kpidPath, IDS_PROPERTY_PATH, 0x02000203 },
  { kpidName, IDS_PROPERTY_NAME, 0x02000204 },
  // { kpidExtension, L"Extension" },
  { kpidIsFolder, IDS_PROPERTY_IS_FOLDER,  0x02000206},
  { kpidSize, IDS_PROPERTY_SIZE, 0x02000207},
  { kpidPackedSize, IDS_PROPERTY_PACKED_SIZE, 0x02000208 }, 
  { kpidAttributes, IDS_PROPERTY_ATTRIBUTES, 0x02000209 },
  { kpidCreationTime, IDS_PROPERTY_CREATION_TIME, 0x0200020A },
  { kpidLastAccessTime, IDS_PROPERTY_LAST_ACCESS_TIME, 0x0200020B },
  { kpidLastWriteTime, IDS_PROPERTY_LAST_WRITE_TIME, 0x0200020C },
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
  { kpidNumSubFolders, IDS_PROPERTY_FOLDERS, 0x0200021F },
  { kpidNumSubFiles, IDS_PROPERTY_FILES, 0x02000220 },
  { kpidUnpackVer, IDS_PROPERTY_VERSION, 0x02000221},
  { kpidVolume, IDS_PROPERTY_VOLUME, 0x02000222},
  { kpidIsVolume, IDS_PROPERTY_IS_VOLUME, 0x02000223},
  { kpidOffset, IDS_PROPERTY_OFFSET, 0x02000224},
  { kpidLinks, IDS_PROPERTY_LINKS, 0x02000225},
  { kpidNumBlocks, IDS_PROPERTY_NUM_BLOCKS, 0x02000226},
  { kpidNumVolumes, IDS_PROPERTY_NUM_VOLUMES, 0x02000227},

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
    if(kPropertyIDNamePairs[i].PropID == propID)
      return i;
  return -1;
}

UString GetNameOfProperty(PROPID propID)
{
  int index = FindProperty(propID);
  if (index < 0)
    return UString();
  const CPropertyIDNamePair &pair = kPropertyIDNamePairs[index];
  return LangString(pair.ResourceID, pair.LangID);
}
