PROG = 7zFM.exe
CFLAGS = $(CFLAGS) -I ../../../ \
  -DLANG \
  -DNEW_FOLDER_INTERFACE \
  -DEXTERNAL_CODECS \

!IFDEF UNDER_CE
LIBS = $(LIBS) ceshell.lib Commctrl.lib
!ELSE
LIBS = $(LIBS) comctl32.lib htmlhelp.lib comdlg32.lib Mpr.lib Gdi32.lib
CFLAGS = $(CFLAGS) -DWIN_LONG_PATH -DSUPPORT_DEVICE_FILE
!ENDIF

FM_OBJS = \
  $O\App.obj \
  $O\BrowseDialog.obj \
  $O\ClassDefs.obj \
  $O\EnumFormatEtc.obj \
  $O\ExtractCallback.obj \
  $O\FileFolderPluginOpen.obj \
  $O\FilePlugins.obj \
  $O\FM.obj \
  $O\FoldersPage.obj \
  $O\FormatUtils.obj \
  $O\FSFolder.obj \
  $O\FSFolderCopy.obj \
  $O\HelpUtils.obj \
  $O\LangUtils.obj \
  $O\MenuPage.obj \
  $O\MyLoadMenu.obj \
  $O\OpenCallback.obj \
  $O\OptionsDialog.obj \
  $O\Panel.obj \
  $O\PanelCopy.obj \
  $O\PanelCrc.obj \
  $O\PanelDrag.obj \
  $O\PanelFolderChange.obj \
  $O\PanelItemOpen.obj \
  $O\PanelItems.obj \
  $O\PanelKey.obj \
  $O\PanelListNotify.obj \
  $O\PanelMenu.obj \
  $O\PanelOperations.obj \
  $O\PanelSelect.obj \
  $O\PanelSort.obj \
  $O\PanelSplitFile.obj \
  $O\ProgramLocation.obj \
  $O\PropertyName.obj \
  $O\RegistryAssociations.obj \
  $O\RegistryPlugins.obj \
  $O\RegistryUtils.obj \
  $O\RootFolder.obj \
  $O\SplitUtils.obj \
  $O\StringUtils.obj \
  $O\SysIconUtils.obj \
  $O\TextPairs.obj \
  $O\UpdateCallback100.obj \
  $O\ViewSettings.obj \
  $O\AboutDialog.obj \
  $O\ComboDialog.obj \
  $O\CopyDialog.obj \
  $O\EditPage.obj \
  $O\LangPage.obj \
  $O\ListViewDialog.obj \
  $O\MessagesDialog.obj \
  $O\OverwriteDialog.obj \
  $O\PasswordDialog.obj \
  $O\ProgressDialog2.obj \
  $O\SettingsPage.obj \
  $O\SplitDialog.obj \
  $O\SystemPage.obj \

COMMON_OBJS = \
  $O\CommandLineParser.obj \
  $O\CRC.obj \
  $O\IntToString.obj \
  $O\Lang.obj \
  $O\ListFileUtils.obj \
  $O\MyMap.obj \
  $O\MyString.obj \
  $O\MyVector.obj \
  $O\MyXml.obj \
  $O\NewHandler.obj \
  $O\Random.obj \
  $O\StringConvert.obj \
  $O\StringToInt.obj \
  $O\TextConfig.obj \
  $O\UTFConvert.obj \
  $O\Wildcard.obj \

WIN_OBJS = \
  $O\Clipboard.obj \
  $O\DLL.obj \
  $O\Error.obj \
  $O\FileDir.obj \
  $O\FileFind.obj \
  $O\FileIO.obj \
  $O\FileName.obj \
  $O\Memory.obj \
  $O\MemoryLock.obj \
  $O\Menu.obj \
  $O\Process.obj \
  $O\PropVariant.obj \
  $O\PropVariantConversions.obj \
  $O\PropVariantUtils.obj \
  $O\Registry.obj \
  $O\ResourceString.obj \
  $O\Shell.obj \
  $O\Synchronization.obj \
  $O\System.obj \
  $O\Time.obj \
  $O\Window.obj \

!IFNDEF UNDER_CE

FM_OBJS = $(FM_OBJS) \
  $O\FSDrives.obj \
  $O\NetFolder.obj \

WIN_OBJS = $(WIN_OBJS) \
  $O\CommonDialog.obj \
  $O\FileSystem.obj \
  $O\Net.obj \
  $O\Security.obj \

!ENDIF


WIN_CTRL_OBJS = \
  $O\ComboBox.obj \
  $O\Dialog.obj \
  $O\ListView.obj \
  $O\PropertyPage.obj \
  $O\Window2.obj \

7ZIP_COMMON_OBJS = \
  $O\CreateCoder.obj \
  $O\CWrappers.obj \
  $O\FilePathAutoRename.obj \
  $O\FileStreams.obj \
  $O\FilterCoder.obj \
  $O\InBuffer.obj \
  $O\InOutTempBuffer.obj \
  $O\LimitedStreams.obj \
  $O\LockedStream.obj \
  $O\MemBlocks.obj \
  $O\MethodId.obj \
  $O\MethodProps.obj \
  $O\OffsetStream.obj \
  $O\OutBuffer.obj \
  $O\OutMemStream.obj \
  $O\ProgressMt.obj \
  $O\ProgressUtils.obj \
  $O\StreamBinder.obj \
  $O\StreamObjects.obj \
  $O\StreamUtils.obj \
  $O\VirtThread.obj \

AR_OBJS = \
  $O\ArjHandler.obj \
  $O\Bz2Handler.obj \
  $O\CpioHandler.obj \
  $O\CramfsHandler.obj \
  $O\DebHandler.obj \
  $O\DeflateProps.obj \
  $O\DmgHandler.obj \
  $O\ElfHandler.obj \
  $O\FatHandler.obj \
  $O\FlvHandler.obj \
  $O\GzHandler.obj \
  $O\LzhHandler.obj \
  $O\LzmaHandler.obj \
  $O\MachoHandler.obj \
  $O\MbrHandler.obj \
  $O\MslzHandler.obj \
  $O\MubHandler.obj \
  $O\NtfsHandler.obj \
  $O\PeHandler.obj \
  $O\PpmdHandler.obj \
  $O\RpmHandler.obj \
  $O\SplitHandler.obj \
  $O\SquashfsHandler.obj \
  $O\SwfHandler.obj \
  $O\VhdHandler.obj \
  $O\XarHandler.obj \
  $O\XzHandler.obj \
  $O\ZHandler.obj \

AR_COMMON_OBJS = \
  $O\CoderMixer2.obj \
  $O\CoderMixer2MT.obj \
  $O\CrossThreadProgress.obj \
  $O\DummyOutStream.obj \
  $O\FindSignature.obj \
  $O\InStreamWithCRC.obj \
  $O\ItemNameUtils.obj \
  $O\MultiStream.obj \
  $O\OutStreamWithCRC.obj \
  $O\OutStreamWithSha1.obj \
  $O\HandlerOut.obj \
  $O\ParseProperties.obj \

UI_COMMON_OBJS = \
  $O\ArchiveCommandLine.obj \
  $O\ArchiveExtractCallback.obj \
  $O\ArchiveName.obj \
  $O\ArchiveOpenCallback.obj \
  $O\Bench.obj \
  $O\CompressCall2.obj \
  $O\DefaultName.obj \
  $O\EnumDirItems.obj \
  $O\Extract.obj \
  $O\ExtractingFilePath.obj \
  $O\LoadCodecs.obj \
  $O\OpenArchive.obj \
  $O\PropIDUtils.obj \
  $O\SetProperties.obj \
  $O\SortUtils.obj \
  $O\TempFiles.obj \
  $O\Update.obj \
  $O\UpdateAction.obj \
  $O\UpdateCallback.obj \
  $O\UpdatePair.obj \
  $O\UpdateProduce.obj \
  $O\WorkDir.obj \
  $O\ZipRegistry.obj \

AGENT_OBJS = \
  $O\Agent.obj \
  $O\AgentOut.obj \
  $O\AgentProxy.obj \
  $O\ArchiveFolder.obj \
  $O\ArchiveFolderOpen.obj \
  $O\ArchiveFolderOut.obj \
  $O\UpdateCallbackAgent.obj \

EXPLORER_OBJS = \
  $O\ContextMenu.obj \
  $O\MyMessages.obj \
  $O\RegistryContextMenu.obj \

GUI_OBJS = \
  $O\BenchmarkDialog.obj \
  $O\CompressDialog.obj \
  $O\ExtractDialog.obj \
  $O\ExtractGUI.obj \
  $O\UpdateCallbackGUI.obj \
  $O\UpdateGUI.obj \


7Z_OBJS = \
  $O\7zCompressionMode.obj \
  $O\7zDecode.obj \
  $O\7zEncode.obj \
  $O\7zExtract.obj \
  $O\7zFolderInStream.obj \
  $O\7zFolderOutStream.obj \
  $O\7zHandler.obj \
  $O\7zHandlerOut.obj \
  $O\7zHeader.obj \
  $O\7zIn.obj \
  $O\7zOut.obj \
  $O\7zProperties.obj \
  $O\7zSpecStream.obj \
  $O\7zUpdate.obj \
  $O\7zRegister.obj \

CAB_OBJS = \
  $O\CabBlockInStream.obj \
  $O\CabHandler.obj \
  $O\CabHeader.obj \
  $O\CabIn.obj \
  $O\CabRegister.obj \

CHM_OBJS = \
  $O\ChmHandler.obj \
  $O\ChmHeader.obj \
  $O\ChmIn.obj \
  $O\ChmRegister.obj \

COM_OBJS = \
  $O\ComHandler.obj \
  $O\ComIn.obj \
  $O\ComRegister.obj \

HFS_OBJS = \
  $O\HfsHandler.obj \
  $O\HfsIn.obj \
  $O\HfsRegister.obj \

ISO_OBJS = \
  $O\IsoHandler.obj \
  $O\IsoHeader.obj \
  $O\IsoIn.obj \
  $O\IsoRegister.obj \

NSIS_OBJS = \
  $O\NsisDecode.obj \
  $O\NsisHandler.obj \
  $O\NsisIn.obj \
  $O\NsisRegister.obj \

RAR_OBJS = \
  $O\RarHandler.obj \
  $O\RarHeader.obj \
  $O\RarIn.obj \
  $O\RarItem.obj \
  $O\RarVolumeInStream.obj \
  $O\RarRegister.obj \

TAR_OBJS = \
  $O\TarHandler.obj \
  $O\TarHandlerOut.obj \
  $O\TarHeader.obj \
  $O\TarIn.obj \
  $O\TarOut.obj \
  $O\TarUpdate.obj \
  $O\TarRegister.obj \

UDF_OBJS = \
  $O\UdfHandler.obj \
  $O\UdfIn.obj \
  $O\UdfRegister.obj \

WIM_OBJS = \
  $O\WimHandler.obj \
  $O\WimHandlerOut.obj \
  $O\WimIn.obj \
  $O\WimRegister.obj \

ZIP_OBJS = \
  $O\ZipAddCommon.obj \
  $O\ZipHandler.obj \
  $O\ZipHandlerOut.obj \
  $O\ZipHeader.obj \
  $O\ZipIn.obj \
  $O\ZipItem.obj \
  $O\ZipOut.obj \
  $O\ZipUpdate.obj \
  $O\ZipRegister.obj \

COMPRESS_OBJS = \
  $O\ArjDecoder1.obj \
  $O\ArjDecoder2.obj \
  $O\Bcj2Coder.obj \
  $O\Bcj2Register.obj \
  $O\BcjCoder.obj \
  $O\BcjRegister.obj \
  $O\BitlDecoder.obj \
  $O\BranchCoder.obj \
  $O\BranchMisc.obj \
  $O\BranchRegister.obj \
  $O\ByteSwap.obj \
  $O\BZip2Crc.obj \
  $O\BZip2Decoder.obj \
  $O\BZip2Encoder.obj \
  $O\BZip2Register.obj \
  $O\CopyCoder.obj \
  $O\CopyRegister.obj \
  $O\Deflate64Register.obj \
  $O\DeflateDecoder.obj \
  $O\DeflateEncoder.obj \
  $O\DeflateRegister.obj \
  $O\DeltaFilter.obj \
  $O\ImplodeDecoder.obj \
  $O\ImplodeHuffmanDecoder.obj \
  $O\LzhDecoder.obj \
  $O\Lzma2Decoder.obj \
  $O\Lzma2Encoder.obj \
  $O\Lzma2Register.obj \
  $O\LzmaDecoder.obj \
  $O\LzmaEncoder.obj \
  $O\LzmaRegister.obj \
  $O\LzOutWindow.obj \
  $O\Lzx86Converter.obj \
  $O\LzxDecoder.obj \
  $O\PpmdDecoder.obj \
  $O\PpmdEncoder.obj \
  $O\PpmdRegister.obj \
  $O\PpmdZip.obj \
  $O\QuantumDecoder.obj \
  $O\Rar1Decoder.obj \
  $O\Rar2Decoder.obj \
  $O\Rar3Decoder.obj \
  $O\Rar3Vm.obj \
  $O\RarCodecsRegister.obj \
  $O\ShrinkDecoder.obj \
  $O\ZlibDecoder.obj \
  $O\ZlibEncoder.obj \
  $O\ZDecoder.obj \

CRYPTO_OBJS = \
  $O\7zAes.obj \
  $O\7zAesRegister.obj \
  $O\HmacSha1.obj \
  $O\MyAes.obj \
  $O\Pbkdf2HmacSha1.obj \
  $O\RandGen.obj \
  $O\Rar20Crypto.obj \
  $O\RarAes.obj \
  $O\Sha1.obj \
  $O\WzAes.obj \
  $O\ZipCrypto.obj \
  $O\ZipStrong.obj \


C_OBJS = \
  $O\7zBuf2.obj \
  $O\7zStream.obj \
  $O\Alloc.obj \
  $O\Bra.obj \
  $O\Bra86.obj \
  $O\BraIA64.obj \
  $O\BwtSort.obj \
  $O\CpuArch.obj \
  $O\Delta.obj \
  $O\HuffEnc.obj \
  $O\LzFind.obj \
  $O\LzFindMt.obj \
  $O\Lzma2Dec.obj \
  $O\Lzma2Enc.obj \
  $O\LzmaDec.obj \
  $O\LzmaEnc.obj \
  $O\MtCoder.obj \
  $O\Ppmd7.obj \
  $O\Ppmd7Dec.obj \
  $O\Ppmd7Enc.obj \
  $O\Ppmd8.obj \
  $O\Ppmd8Dec.obj \
  $O\Ppmd8Enc.obj \
  $O\Sha256.obj \
  $O\Sort.obj \
  $O\Threads.obj \
  $O\Xz.obj \
  $O\XzCrc64.obj \
  $O\XzDec.obj \
  $O\XzEnc.obj \
  $O\XzIn.obj \

!include "../../Aes.mak"
!include "../../Crc.mak"

OBJS = \
  $O\StdAfx.obj \
  $(FM_OBJS)\
  $(COMMON_OBJS) \
  $(WIN_OBJS) \
  $(WIN_CTRL_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(AR_OBJS) \
  $(AR_COMMON_OBJS) \
  $(UI_COMMON_OBJS) \
  $(AGENT_OBJS) \
  $(EXPLORER_OBJS) \
  $(GUI_OBJS) \
  $(7Z_OBJS) \
  $(CAB_OBJS) \
  $(CHM_OBJS) \
  $(COM_OBJS) \
  $(HFS_OBJS) \
  $(ISO_OBJS) \
  $(NSIS_OBJS) \
  $(RAR_OBJS) \
  $(TAR_OBJS) \
  $(UDF_OBJS) \
  $(WIM_OBJS) \
  $(ZIP_OBJS) \
  $(COMPRESS_OBJS) \
  $(CRYPTO_OBJS) \
  $(C_OBJS) \
  $(ASM_OBJS) \
  $O\resource.res \

!include "../../../Build.mak"

$(FM_OBJS): ../../UI/FileManager/$(*B).cpp
	$(COMPL)
$(COMMON_OBJS): ../../../Common/$(*B).cpp
	$(COMPL)
$(WIN_OBJS): ../../../Windows/$(*B).cpp
	$(COMPL)
$(WIN_CTRL_OBJS): ../../../Windows/Control/$(*B).cpp
	$(COMPL)
$(7ZIP_COMMON_OBJS): ../../Common/$(*B).cpp
	$(COMPL)

$(AR_OBJS): ../../Archive/$(*B).cpp
	$(COMPL)
$(AR_COMMON_OBJS): ../../Archive/Common/$(*B).cpp
	$(COMPL)

$(7Z_OBJS): ../../Archive/7z/$(*B).cpp
	$(COMPL)
$(CAB_OBJS): ../../Archive/Cab/$(*B).cpp
	$(COMPL)
$(CHM_OBJS): ../../Archive/Chm/$(*B).cpp
	$(COMPL)
$(COM_OBJS): ../../Archive/Com/$(*B).cpp
	$(COMPL)
$(HFS_OBJS): ../../Archive/Hfs/$(*B).cpp
	$(COMPL)
$(ISO_OBJS): ../../Archive/Iso/$(*B).cpp
	$(COMPL)
$(NSIS_OBJS): ../../Archive/Nsis/$(*B).cpp
	$(COMPL)
$(RAR_OBJS): ../../Archive/Rar/$(*B).cpp
	$(COMPL)
$(TAR_OBJS): ../../Archive/Tar/$(*B).cpp
	$(COMPL)
$(UDF_OBJS): ../../Archive/Udf/$(*B).cpp
	$(COMPL)
$(WIM_OBJS): ../../Archive/Wim/$(*B).cpp
	$(COMPL)
$(ZIP_OBJS): ../../Archive/Zip/$(*B).cpp
	$(COMPL)

$(COMPRESS_OBJS): ../../Compress/$(*B).cpp
	$(COMPL_O2)

$(CRYPTO_OBJS): ../../Crypto/$(*B).cpp
	$(COMPL_O2)

$(UI_COMMON_OBJS): ../../UI/Common/$(*B).cpp
	$(COMPL)
$(AGENT_OBJS): ../../UI/Agent/$(*B).cpp
	$(COMPL)
$(EXPLORER_OBJS): ../../UI/Explorer/$(*B).cpp
	$(COMPL)
$(GUI_OBJS): ../../UI/GUI/$(*B).cpp
	$(COMPL)

$(C_OBJS): ../../../../C/$(*B).c
	$(COMPL_O2)

!include "../../Asm.mak"
