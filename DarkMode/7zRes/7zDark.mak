!IFDEF MAK_SINGLE_FILE

!IFDEF DARK_MODE_OBJS
$(DARK_MODE_OBJS): ../../../../DarkMode/lib/src/$(*B).cpp
	$(COMPL_O2) -I../../../../DarkMode/lib/include
!ENDIF

!ELSE

{../../../../DarkMode/lib/src}.cpp{$O}.obj::
	$(COMPLB_O2) -I../../../../DarkMode/lib/include

!ENDIF
