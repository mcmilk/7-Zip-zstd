
!IFNDEF MY_NO_UNICODE
CFLAGS = $(CFLAGS) -DUNICODE -D_UNICODE
!ENDIF

!IFNDEF O
!IFDEF PLATFORM
O=$(PLATFORM)
!ELSE
O=o
!ENDIF
!ENDIF

LIBS = $(LIBS) oleaut32.lib ole32.lib user32.lib advapi32.lib shell32.lib

CFLAGS = $(CFLAGS) -c /nologo /Fo$O/ /W4 /WX /EHsc /MT /MP /GR- /GL /Gw /Gy

!IFDEF MY_CONSOLE
CFLAGS = $(CFLAGS) -D_CONSOLE
!ENDIF

CFLAGS_O1 = $(CFLAGS) /O1
CFLAGS_O2 = $(CFLAGS) /O2 /Ob3

LFLAGS = $(LFLAGS) /nologo /LTCG /LARGEADDRESSAWARE

!IFDEF DEF_FILE
LFLAGS = $(LFLAGS) /DLL /DEF:$(DEF_FILE)
!ENDIF

PROGPATH = $O\$(PROG)

!IF "$(PLATFORM)" == "x64"
MY_ML = ml64 /Dx64 /WX
!ELSEIF "$(PLATFORM)" == "arm"
MY_ML = armasm /WX
!ELSE
MY_ML = ml /WX
!ENDIF

!IF "$(PLATFORM)" == "arm"
COMPL_ASM = $(MY_ML) /nologo $** $O/$(*B).obj
!ELSE
COMPL_ASM = $(MY_ML) /nologo -c /Fo$O/ $**
!ENDIF

COMPL_O1     = $(CC) $(CFLAGS_O1) $**
COMPL_O2     = $(CC) $(CFLAGS_O2) $**
COMPL_PCH    = $(CC) $(CFLAGS_O1) /Yc"StdAfx.h" /Fp$O/a.pch $**
COMPL        = $(CC) $(CFLAGS_O1) /Yu"StdAfx.h" /Fp$O/a.pch $**
COMPLB       = $(CC) $(CFLAGS_O1) /Yu"StdAfx.h" /Fp$O/a.pch $<
COMPLB_O2    = $(CC) $(CFLAGS_O2) $<

CFLAGS_C_ALL = $(CFLAGS_O2) $(CFLAGS_C_SPEC)
CCOMPL_PCH   = $(CC) $(CFLAGS_C_ALL) /Yc"Precomp.h" /Fp$O/a.pch $**
CCOMPL_USE   = $(CC) $(CFLAGS_C_ALL) /Yu"Precomp.h" /Fp$O/a.pch $**
CCOMPL       = $(CC) $(CFLAGS_C_ALL) $**
CCOMPLB      = $(CC) $(CFLAGS_C_ALL) $<


all: $(PROGPATH)

clean:
	-del /Q $(PROGPATH) $O\*.exe $O\*.dll $O\*.obj $O\*.lib $O\*.exp $O\*.res $O\*.pch $O\*.asm

$O:
	if not exist "$O" mkdir "$O"
$O/asm:
	if not exist "$O/asm" mkdir "$O/asm"

$(PROGPATH): $O $O/asm $(OBJS) $(DEF_FILE)
	link $(LFLAGS) -out:$(PROGPATH) $(OBJS) $(LIBS)

!IFNDEF NO_DEFAULT_RES
$O\resource.res: $(*B).rc
	rc $(RFLAGS) /nologo /fo$@ $**
!ENDIF
$O\StdAfx.obj: $(*B).cpp
	$(COMPL_PCH)
