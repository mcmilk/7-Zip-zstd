// SevenZip/ Messages.h

#ifndef __SEVENZIP_MESSAGES_H
#define __SEVENZIP_MESSAGES_H

namespace NMessageID {

enum EEnum
{
  kOk,     
  kCancel,

  kWarning,
  kError,

  kArchiveType,

  kProperties,

  kYes,
  kNo,
  
  kName,
  kExtension,
  kIsFolder,
  kSize,
  kPackedSize,
  kAttributes,
  kCreationTime,
  kLastAccessTime,
  kLastWriteTime,
  kSolid, 
  kCommented, 
  kEncrypted, 
  kSplitBefore, 
  kSplitAfter,
  kDictionarySize, 
  kCRC, 
  kType,
  kAnti,
  kMethod, 
  kHostOS, 
  kFileSystem,
  kUser,
  kGroup,
  kBlock,
  kComment,
  kPosition,
  kNumSubFolders,
  kNumSubFiles,
  kUnpackVer,
  kVolume,
  kIsVolume,
  kOffset,
  kLinks,
  kNumBlocks,
  kNumVolumes,

  kGetPasswordTitle,
  kEnterPasswordForFile,

  kExtractTitle,
  kExtractTo,

  kExtractPathMode,
  kExtractPathFull,
  kExtractPathCurrent,
  kExtractPathNo,

  kExtractOwerwriteMode,
  kExtractOwerwriteAsk,
  kExtractOwerwritePrompt,
  kExtractOwerwriteSkip,
  kExtractOwerwriteAutoRename,
  kExtractOwerwriteAutoRenameExisting,

  kExtractFilesMode,
  kExtractFilesSelected,
  kExtractFilesAll,

  kExtractPassword,

  kExtractExtract,
  kExtractCancel,

  kExtractCanNotOpenOutputFile,

  kExtractUnsupportedMethod,
  kExtractCRCFailed,
  kExtractDataError,
  kExtractCRCFailedEncrypted,
  kExtractDataErrorEncrypted,

  kOverwriteTitle,
  kOverwriteMessage1,
  kOverwriteMessageWouldYouLike,
  kOverwriteMessageWithtTisOne,

  kOverwriteBytes,
  kOverwriteModifiedOn,

  kOverwriteYes,
  kOverwriteYesToAll,
  kOverwriteNo,
  kOverwriteNoToAll,
  kOverwriteAutoRename,
  kOverwriteCancel,

  kUpdateNotSupportedForThisArchive,

  kDeleteTitle,
  kDeleteFile,
  kDeleteFiles,
  kDeleteNumberOfFiles,
  kDeleteDelete,
  kDeleteCancel,

  kUpdateTitle,
  kUpdateAddToArchive,

  kUpdateMethod,
  kUpdateMethodStore,
  kUpdateMethodFastest,
  kUpdateMethodFast,
  kUpdateMethodNormal,
  kUpdateMethodMaximum,
  kUpdateMethodUltra,

  kUpdateMode,
  kUpdateModeAdd,
  kUpdateModeUpdate,
  kUpdateModeFreshen,
  kUpdateModeSynchronize,

  kUpdateAdd,
  kUpdateSelectArchiver,

  kUpdateSelectArchiverMenuTitle,

  // kArcReadFiles,
  
  kWaitTitle,
  
  kReading,
  kExtracting,
  kDeleting,
  kUpdating,
  
  // kReadingList,

  kMoveIsNotSupported,

  kOpenArchiveMenuString,
  kCreateArchiveMenuString,

  kConfigTitle,
  
  kConfigPluginEnabled
};

}

#endif