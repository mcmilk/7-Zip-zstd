CRC_OBJS = \
!IF "$(CPU)" != "IA64"
  $O\7zCrcT8U.obj \
  $O\7zCrcT8.obj \
!ELSE
  $O\7zCrc.obj \
!ENDIF
