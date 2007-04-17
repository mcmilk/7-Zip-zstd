# Microsoft Developer Studio Project File - Name="Branch" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=Branch - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Branch.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Branch.mak" CFG="Branch - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Branch - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Branch - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Branch - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "BRANCH_EXPORTS" /YX /FD /c
# ADD CPP /nologo /Gz /MD /W3 /GX /O2 /I "..\..\..\\" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "BRANCH_EXPORTS" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"C:\Program Files\7-Zip\Codecs\Branch.dll" /opt:NOWIN98
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "Branch - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "BRANCH_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /Gz /MTd /W3 /Gm /GX /ZI /Od /I "..\..\..\\" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "BRANCH_EXPORTS" /Yu"StdAfx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"C:\Program Files\7-Zip\Codecs\Branch.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "Branch - Win32 Release"
# Name "Branch - Win32 Debug"
# Begin Group "Spec"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Codec.def
# End Source File
# Begin Source File

SOURCE=..\CodecExports.cpp
# End Source File
# Begin Source File

SOURCE=..\DllExports.cpp
# End Source File
# Begin Source File

SOURCE=.\resource.rc
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"StdAfx.h"
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Methods"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ARM.cpp

!IF  "$(CFG)" == "Branch - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Branch - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ARM.h
# End Source File
# Begin Source File

SOURCE=.\ARMThumb.cpp

!IF  "$(CFG)" == "Branch - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Branch - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ARMThumb.h
# End Source File
# Begin Source File

SOURCE=.\BCJ2Register.cpp
# End Source File
# Begin Source File

SOURCE=.\BCJRegister.cpp
# End Source File
# Begin Source File

SOURCE=.\BranchCoder.cpp
# End Source File
# Begin Source File

SOURCE=.\BranchCoder.h
# End Source File
# Begin Source File

SOURCE=.\BranchRegister.cpp
# End Source File
# Begin Source File

SOURCE=.\BranchTypes.h
# End Source File
# Begin Source File

SOURCE=.\BranchX86.h
# End Source File
# Begin Source File

SOURCE=.\IA64.cpp

!IF  "$(CFG)" == "Branch - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Branch - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\IA64.h
# End Source File
# Begin Source File

SOURCE=.\PPC.cpp

!IF  "$(CFG)" == "Branch - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Branch - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PPC.h
# End Source File
# Begin Source File

SOURCE=.\SPARC.cpp
# End Source File
# Begin Source File

SOURCE=.\SPARC.h
# End Source File
# Begin Source File

SOURCE=.\x86.cpp

!IF  "$(CFG)" == "Branch - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Branch - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\x86.h
# End Source File
# Begin Source File

SOURCE=.\x86_2.cpp

!IF  "$(CFG)" == "Branch - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Branch - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\x86_2.h
# End Source File
# End Group
# Begin Group "Stream"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Common\InBuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\InBuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\OutBuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\OutBuffer.h
# End Source File
# End Group
# Begin Group "RangeCoder"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\RangeCoder\RangeCoder.h
# End Source File
# Begin Source File

SOURCE=..\RangeCoder\RangeCoderBit.h
# End Source File
# End Group
# Begin Group "C"

# PROP Default_Filter ""
# Begin Group "C Branch"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchARM.c

!IF  "$(CFG)" == "Branch - Win32 Release"

!ELSEIF  "$(CFG)" == "Branch - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchARM.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchARMThumb.c

!IF  "$(CFG)" == "Branch - Win32 Release"

!ELSEIF  "$(CFG)" == "Branch - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchARMThumb.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchIA64.c

!IF  "$(CFG)" == "Branch - Win32 Release"

!ELSEIF  "$(CFG)" == "Branch - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchIA64.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchPPC.c

!IF  "$(CFG)" == "Branch - Win32 Release"

!ELSEIF  "$(CFG)" == "Branch - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchPPC.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchSPARC.c

!IF  "$(CFG)" == "Branch - Win32 Release"

!ELSEIF  "$(CFG)" == "Branch - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchSPARC.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchTypes.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchX86.c

!IF  "$(CFG)" == "Branch - Win32 Release"

!ELSEIF  "$(CFG)" == "Branch - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchX86.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\..\..\C\Alloc.c

!IF  "$(CFG)" == "Branch - Win32 Release"

!ELSEIF  "$(CFG)" == "Branch - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Alloc.h
# End Source File
# End Group
# End Target
# End Project
