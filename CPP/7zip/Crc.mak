!IF "$(CPU)" != "IA64"
!IF "$(CPU)" != "AMD64"
$O\7zCrcT8U.obj: ../../../../Asm/x86/$(*B).asm
	$(COMPL_ASM)
!ELSE
$O\7zCrcT8U.obj: ../../../../Asm/x64/$(*B).asm
	$(COMPL_ASM)
!ENDIF
$O\7zCrcT8.obj: ../../../../C/$(*B).c
	$(COMPL_O2)
!ELSE
$(CRC_OBJS): ../../../../C/$(*B).c
	$(COMPL_O2)
!ENDIF

