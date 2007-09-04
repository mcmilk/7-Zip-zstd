PROG = 7-zip.dll
DEF_FILE = Explorer.def
LIBS = $(LIBS) user32.lib oleaut32.lib advapi32.lib htmlhelp.lib shell32.lib comctl32.lib ole32.lib comdlg32.lib
CFLAGS = $(CFLAGS) -I ../../../ -Ob2 \
  -DLANG \
  -DWIN_LONG_PATH \

EXPLORER_OBJS = \
  $O\DllExports.obj \
  $O\ContextMenu.obj \
  $O\MyMessages.obj \
  $O\OptionsDialog.obj \
  $O\RegistryContextMenu.obj \
  $O\SystemPage.obj \
  $O\FoldersPage.obj \

COMMON_OBJS = \
  $O\IntToString.obj \
  $O\Lang.obj \
  $O\MyString.obj \
  $O\MyVector.obj \
  $O\NewHandler.obj \
  $O\Random.obj \
  $O\StringConvert.obj \
  $O\StringToInt.obj \
  $O\TextConfig.obj \
  $O\UTFConvert.obj \
  $O\Wildcard.obj \

WIN_OBJS = \
  $O\DLL.obj \
  $O\Error.obj \
  $O\FileDir.obj \
  $O\FileFind.obj \
  $O\FileIO.obj \
  $O\FileName.obj \
  $O\Menu.obj \
  $O\Registry.obj \
  $O\ResourceString.obj \
  $O\Shell.obj \
  $O\Synchronization.obj \
  $O\Window.obj \

WIN_CTRL_OBJS = \
  $O\Dialog.obj \
  $O\PropertyPage.obj \

UI_COMMON_OBJS = \
  $O\ArchiveName.obj \
  $O\CompressCall.obj \
  $O\ZipRegistry.obj \

FM_COMMON_OBJS = \
  $O\FormatUtils.obj \
  $O\HelpUtils.obj \
  $O\LangUtils.obj \
  $O\ProgramLocation.obj \
  $O\RegistryUtils.obj \

C_OBJS = \
  $O\Threads.obj \

OBJS = \
  $O\StdAfx.obj \
  $(EXPLORER_OBJS) \
  $(COMMON_OBJS) \
  $(WIN_OBJS) \
  $(WIN_CTRL_OBJS) \
  $(UI_COMMON_OBJS) \
  $(FM_COMMON_OBJS)\
  $(C_OBJS) \
  $O\resource.res

!include "../../../Build.mak"

$(EXPLORER_OBJS): $(*B).cpp
	$(COMPL)
$(COMMON_OBJS): ../../../Common/$(*B).cpp
	$(COMPL)
$(WIN_OBJS): ../../../Windows/$(*B).cpp
	$(COMPL)
$(WIN_CTRL_OBJS): ../../../Windows/Control/$(*B).cpp
	$(COMPL)
$(UI_COMMON_OBJS): ../Common/$(*B).cpp
	$(COMPL)
$(FM_COMMON_OBJS): ../FileManager/$(*B).cpp
	$(COMPL)
$(C_OBJS): ../../../../C/$(*B).c
	$(COMPL_O2)
