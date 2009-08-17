PROG = 7-zip.dll
DEF_FILE = Explorer.def
CFLAGS = $(CFLAGS) -I ../../../ \
  -DLANG \

!IFDEF UNDER_CE
LIBS = $(LIBS) Commctrl.lib
!ELSE
LIBS = $(LIBS) comctl32.lib htmlhelp.lib comdlg32.lib Mpr.lib Gdi32.lib
CFLAGS = $(CFLAGS) -DWIN_LONG_PATH
!ENDIF

EXPLORER_OBJS = \
  $O\DllExports.obj \
  $O\ContextMenu.obj \
  $O\MyMessages.obj \
  $O\RegistryContextMenu.obj \

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
  $O\Process.obj \
  $O\Registry.obj \
  $O\ResourceString.obj \
  $O\Shell.obj \
  $O\Synchronization.obj \
  $O\Window.obj \

!IFDEF UNDER_CE

WIN_OBJS = $(WIN_OBJS) \
  $O\CommonDialog.obj \

!ENDIF

WIN_CTRL_OBJS = \
  $O\Dialog.obj \
  $O\ListView.obj \
  $O\PropertyPage.obj \

UI_COMMON_OBJS = \
  $O\ArchiveName.obj \
  $O\CompressCall.obj \
  $O\ExtractingFilePath.obj \
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
