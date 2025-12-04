!IFDEF MAK_SINGLE_FILE

!IFDEF DARK_MODE_OBJS
$(DARK_MODE_OBJS): ../../../../DarkMode/src/$(*B).cpp
	$(COMPL)
!ENDIF

!ELSE

{../../../../DarkMode/src}.cpp{$O}.obj::
	$(COMPLB)

!ENDIF
