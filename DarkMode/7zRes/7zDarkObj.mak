!IFDEF WIN_CTRL_OBJS
DARK_MODE_OBJS = \
  $O\DarkModeSubclass.obj \
  $O\DmlibColor.obj \
  $O\DmlibDpi.obj \
  $O\DmlibHook.obj \
  $O\DmlibIni.obj \
  $O\DmlibPaintHelper.obj \
  $O\DmlibSubclass.obj \
  $O\DmlibSubclassControl.obj \
  $O\DmlibSubclassWindow.obj \
  $O\DmlibWinApi.obj \
!ENDIF

OBJS = \
  $(OBJS) \
  $(DARK_MODE_OBJS) \
