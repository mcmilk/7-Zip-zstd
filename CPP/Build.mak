!IFDEF CPU
LIBS = $(LIBS) bufferoverflowU.lib 
CFLAGS = $(CFLAGS) -GS- -Zc:forScope
!ENDIF

!IFNDEF O
!IFDEF CPU
O=$(CPU)
!ELSE
O=O
!ENDIF
!ENDIF

!IF "$(CPU)" != "IA64"
!IF "$(CPU)" != "AMD64"
MY_ML = ml
!ELSE
MY_ML = ml64
!ENDIF
!ENDIF

COMPL_ASM = $(MY_ML) -c -Fo$O/ $**

CFLAGS = $(CFLAGS) -nologo -c -Fo$O/ -EHsc -Gz -WX -Gy

!IFDEF MY_STATIC_LINK
!IFNDEF MY_SINGLE_THREAD
CFLAGS = $(CFLAGS) -MT
!ENDIF
!ELSE
CFLAGS = $(CFLAGS) -MD
!ENDIF

!IFDEF NEW_COMPILER
CFLAGS_O1 = $(CFLAGS) -O1 -W4 -Wp64
CFLAGS_O2 = $(CFLAGS) -O2 -W4 -Wp64
!ELSE
CFLAGS_O1 = $(CFLAGS) -O1 -W3
CFLAGS_O2 = $(CFLAGS) -O2 -W3
!ENDIF

LFLAGS = $(LFLAGS) -nologo -OPT:NOWIN98 -OPT:REF 

!IFDEF DEF_FILE
LFLAGS = $(LFLAGS) -DLL -DEF:$(DEF_FILE)
!ENDIF

PROGPATH = $O\$(PROG)

COMPL_O1   = $(CPP) $(CFLAGS_O1) $**
COMPL_O2   = $(CPP) $(CFLAGS_O2) $**
COMPL_PCH  = $(CPP) $(CFLAGS_O1) -Yc"StdAfx.h" -Fp$O/a.pch $** 
COMPL      = $(CPP) $(CFLAGS_O1) -Yu"StdAfx.h" -Fp$O/a.pch $**

all: $(PROGPATH) 

clean:
	-del /Q $(PROGPATH) $O\*.exe $O\*.dll $O\*.obj $O\*.lib $O\*.exp $O\*.res $O\*.pch 

$O:
	if not exist "$O" mkdir "$O"

$(PROGPATH): $O $(OBJS) $(DEF_FILE)
	link $(LFLAGS) -out:$(PROGPATH) $(OBJS) $(LIBS)
$O\resource.res: $(*B).rc
	rc -fo$@ $**
$O\StdAfx.obj: $(*B).cpp
	$(COMPL_PCH)
