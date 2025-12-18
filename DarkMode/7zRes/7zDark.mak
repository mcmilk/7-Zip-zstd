!IFDEF MAK_SINGLE_FILE

!IFDEF DARK_MODE_OBJS
$(DARK_MODE_OBJS): ../../../../DarkMode/lib/src/$(*B).cpp
	$(COMPL) -I../../../../DarkMode/lib/include
!ENDIF

!ELSE

{../../../../DarkMode/lib/src}.cpp{$O}.obj::
	$(COMPLB) -I../../../../DarkMode/lib/include

!ENDIF
