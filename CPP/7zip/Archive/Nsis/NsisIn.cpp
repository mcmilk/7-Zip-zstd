// Archive/NsisIn.cpp

#include "StdAfx.h"

#include "NsisIn.h"
#include "NsisDecode.h"

#include "Windows/Defs.h"

#include "../../Common/StreamUtils.h"

#include "Common/IntToString.h"

namespace NArchive {
namespace NNsis {

Byte kSignature[kSignatureSize] = { 0xEF + 1, 0xBE, 0xAD, 0xDE, 
0x4E, 0x75, 0x6C, 0x6C, 0x73, 0x6F, 0x66, 0x74, 0x49, 0x6E, 0x73, 0x74};

class SignatureInitializer
{
public:
  SignatureInitializer() { kSignature[0]--; };
} g_SignatureInitializer;

#ifdef NSIS_SCRIPT
static const char *kCrLf = "\x0D\x0A";
#endif

UInt32 GetUInt32FromMemLE(const Byte *p)
{
  return p[0] | (((UInt32)p[1]) << 8) | (((UInt32)p[2]) << 16) | (((UInt32)p[3]) << 24);
}

Byte CInArchive::ReadByte()
{
  if (_posInData >= _size)
    throw 1;
  return _data[_posInData++];
}

UInt32 CInArchive::ReadUInt32()
{
  UInt32 value = 0;
  for (int i = 0; i < 4; i++)
    value |= ((UInt32)(ReadByte()) << (8 * i));
  return value;
}

void CInArchive::ReadBlockHeader(CBlockHeader &bh)
{
  bh.Offset = ReadUInt32();
  bh.Num = ReadUInt32();
}

#define RINOZ(x) { int __tt = (x); if (__tt != 0) return __tt; }

static int CompareItems(void *const *p1, void *const *p2, void * /* param */)
{
  const CItem &i1 = **(CItem **)p1;
  const CItem &i2 = **(CItem **)p2;
  RINOZ(MyCompare(i1.Pos, i2.Pos));
  RINOZ(i1.Prefix.Compare(i2.Prefix));
  RINOZ(i1.Name.Compare(i2.Name));
  return 0;
}

AString CInArchive::ReadString(UInt32 pos)
{
  AString s;
  UInt32 offset = GetOffset() + _stringsPos + pos;
  for (;;)
  {
    if (offset >= _size)
      throw 1;
    char c = _data[offset++];
    if (c == 0)
      break;
    s += c;
  }
  return s;
}

/*
static AString ParsePrefix(const AString &prefix)
{
  AString res = prefix;
  if (prefix.Length() >= 3)
  {
    if ((Byte)prefix[0] == 0xFD && (Byte)prefix[1] == 0x95 && (Byte)prefix[2] == 0x80)
      res = "$INSTDIR" + prefix.Mid(3);
    else if ((Byte)prefix[0] == 0xFD && (Byte)prefix[1] == 0x96 && (Byte)prefix[2] == 0x80)
      res = "$OUTDIR" + prefix.Mid(3);
  }
  return res;
}
*/

#define SYSREGKEY "Software\\Microsoft\\Windows\\CurrentVersion"

/*
#  define CSIDL_PROGRAMS 0x2
#  define CSIDL_PRINTERS 0x4
#  define CSIDL_PERSONAL 0x5
#  define CSIDL_FAVORITES 0x6
#  define CSIDL_STARTUP 0x7
#  define CSIDL_RECENT 0x8
#  define CSIDL_SENDTO 0x9
#  define CSIDL_STARTMENU 0xB
#  define CSIDL_MYMUSIC 0xD
#  define CSIDL_MYVIDEO 0xE

#  define CSIDL_DESKTOPDIRECTORY 0x10
#  define CSIDL_NETHOOD 0x13
#  define CSIDL_FONTS 0x14
#  define CSIDL_TEMPLATES 0x15
#  define CSIDL_COMMON_STARTMENU 0x16
#  define CSIDL_COMMON_PROGRAMS 0x17
#  define CSIDL_COMMON_STARTUP 0x18
#  define CSIDL_COMMON_DESKTOPDIRECTORY 0x19
#  define CSIDL_APPDATA 0x1A
#  define CSIDL_PRINTHOOD 0x1B
#  define CSIDL_LOCAL_APPDATA 0x1C
#  define CSIDL_ALTSTARTUP 0x1D
#  define CSIDL_COMMON_ALTSTARTUP 0x1E
#  define CSIDL_COMMON_FAVORITES 0x1F

#  define CSIDL_INTERNET_CACHE 0x20
#  define CSIDL_COOKIES 0x21
#  define CSIDL_HISTORY 0x22
#  define CSIDL_COMMON_APPDATA 0x23
#  define CSIDL_WINDOWS 0x24
#  define CSIDL_SYSTEM 0x25
#  define CSIDL_PROGRAM_FILES 0x26
#  define CSIDL_MYPICTURES 0x27
#  define CSIDL_PROFILE 0x28
#  define CSIDL_PROGRAM_FILES_COMMON 0x2B
#  define CSIDL_COMMON_TEMPLATES 0x2D
#  define CSIDL_COMMON_DOCUMENTS 0x2E
#  define CSIDL_COMMON_ADMINTOOLS 0x2F

#  define CSIDL_ADMINTOOLS 0x30
#  define CSIDL_COMMON_MUSIC 0x35
#  define CSIDL_COMMON_PICTURES 0x36
#  define CSIDL_COMMON_VIDEO 0x37
#  define CSIDL_RESOURCES 0x38
#  define CSIDL_RESOURCES_LOCALIZED 0x39
#  define CSIDL_CDBURN_AREA 0x3B
*/

struct CCommandPair
{
  int NumParams;
  const char *Name;
};

enum
{
  // 0
  EW_INVALID_OPCODE,    // zero is invalid. useful for catching errors. (otherwise an all zeroes instruction
                        // does nothing, which is easily ignored but means something is wrong.
  EW_RET,               // return from function call
  EW_NOP,               // Nop/Jump, do nothing: 1, [?new address+1:advance one]
  EW_ABORT,             // Abort: 1 [status]
  EW_QUIT,              // Quit: 0
  EW_CALL,              // Call: 1 [new address+1]
  EW_UPDATETEXT,        // Update status text: 2 [update str, ui_st_updateflag=?ui_st_updateflag:this]
  EW_SLEEP,             // Sleep: 1 [sleep time in milliseconds]
  EW_BRINGTOFRONT,      // BringToFront: 0
  EW_CHDETAILSVIEW,     // SetDetailsView: 2 [listaction,buttonaction]
  
  // 10
  EW_SETFILEATTRIBUTES, // SetFileAttributes: 2 [filename, attributes]
  EW_CREATEDIR,         // Create directory: 2, [path, ?update$INSTDIR]
  EW_IFFILEEXISTS,      // IfFileExists: 3, [file name, jump amount if exists, jump amount if not exists]
  EW_SETFLAG,           // Sets a flag: 2 [id, data]
  EW_IFFLAG,            // If a flag: 4 [on, off, id, new value mask]
  EW_GETFLAG,           // Gets a flag: 2 [output, id]
  EW_RENAME,            // Rename: 3 [old, new, rebootok]
  EW_GETFULLPATHNAME,   // GetFullPathName: 2 [output, input, ?lfn:sfn]
  EW_SEARCHPATH,        // SearchPath: 2 [output, filename]
  EW_GETTEMPFILENAME,   // GetTempFileName: 2 [output, base_dir]
  
  // 20
  EW_EXTRACTFILE,       // File to extract: 6 [overwriteflag, output filename, compressed filedata, filedatetimelow, filedatetimehigh, allow ignore]
                        //  overwriteflag: 0x1 = no. 0x0=force, 0x2=try, 0x3=if date is newer
  EW_DELETEFILE,        // Delete File: 2, [filename, rebootok]
  EW_MESSAGEBOX,        // MessageBox: 5,[MB_flags,text,retv1:retv2,moveonretv1:moveonretv2]
  EW_RMDIR,             // RMDir: 2 [path, recursiveflag]
  EW_STRLEN,            // StrLen: 2 [output, input]
  EW_ASSIGNVAR,         // Assign: 4 [variable (0-9) to assign, string to assign, maxlen, startpos]
  EW_STRCMP,            // StrCmp: 5 [str1, str2, jump_if_equal, jump_if_not_equal, case-sensitive?]
  EW_READENVSTR,        // ReadEnvStr/ExpandEnvStrings: 3 [output, string_with_env_variables, IsRead]
  EW_INTCMP,            // IntCmp: 6 [val1, val2, equal, val1<val2, val1>val2, unsigned?]
  EW_INTOP,             // IntOp: 4 [output, input1, input2, op] where op: 0=add, 1=sub, 2=mul, 3=div, 4=bor, 5=band, 6=bxor, 7=bnot input1, 8=lnot input1, 9=lor, 10=land], 11=1%2
  
  // 30
  EW_INTFMT,            // IntFmt: [output, format, input]
  EW_PUSHPOP,           // Push/Pop/Exchange: 3 [variable/string, ?pop:push, ?exch]
  EW_FINDWINDOW,        // FindWindow: 5, [outputvar, window class,window name, window_parent, window_after]
  EW_SENDMESSAGE,       // SendMessage: 6 [output, hwnd, msg, wparam, lparam, [wparamstring?1:0 | lparamstring?2:0 | timeout<<2]
  EW_ISWINDOW,          // IsWindow: 3 [hwnd, jump_if_window, jump_if_notwindow]
  EW_GETDLGITEM,        // GetDlgItem:        3: [outputvar, dialog, item_id]
  EW_SETCTLCOLORS,      // SerCtlColors:      3: [hwnd, pointer to struct colors]
  EW_SETBRANDINGIMAGE,  // SetBrandingImage:  1: [Bitmap file]
  EW_CREATEFONT,        // CreateFont:        5: [handle output, face name, height, weight, flags]
  EW_SHOWWINDOW,        // ShowWindow:        2: [hwnd, show state]
  
  // 40
  EW_SHELLEXEC,         // ShellExecute program: 4, [shell action, complete commandline, parameters, showwindow]
  EW_EXECUTE,           // Execute program: 3,[complete command line,waitflag,>=0?output errorcode]
  EW_GETFILETIME,       // GetFileTime; 3 [file highout lowout]
  EW_GETDLLVERSION,     // GetDLLVersion: 3 [file highout lowout]
  EW_REGISTERDLL,       // Register DLL: 3,[DLL file name, string ptr of function to call, text to put in display (<0 if none/pass parms), 1 - no unload, 0 - unload]
  EW_CREATESHORTCUT,    // Make Shortcut: 5, [link file, target file, parameters, icon file, iconindex|show mode<<8|hotkey<<16]
  EW_COPYFILES,         // CopyFiles: 3 [source mask, destination location, flags]
  EW_REBOOT,            // Reboot: 0
  EW_WRITEINI,          // Write INI String: 4, [Section, Name, Value, INI File]
  EW_READINISTR,        // ReadINIStr: 4 [output, section, name, ini_file]
  
  // 50
  EW_DELREG,            // DeleteRegValue/DeleteRegKey: 4, [root key(int), KeyName, ValueName, delkeyonlyifempty]. ValueName is -1 if delete key
  EW_WRITEREG,          // Write Registry value: 5, [RootKey(int),KeyName,ItemName,ItemData,typelen]
                        //  typelen=1 for str, 2 for dword, 3 for binary, 0 for expanded str
  EW_READREGSTR,        // ReadRegStr: 5 [output, rootkey(int), keyname, itemname, ==1?int::str]
  EW_REGENUM,           // RegEnum: 5 [output, rootkey, keyname, index, ?key:value]
  EW_FCLOSE,            // FileClose: 1 [handle]
  EW_FOPEN,             // FileOpen: 4  [name, openmode, createmode, outputhandle]
  EW_FPUTS,             // FileWrite: 3 [handle, string, ?int:string]
  EW_FGETS,             // FileRead: 4  [handle, output, maxlen, ?getchar:gets]
  EW_FSEEK,             // FileSeek: 4  [handle, offset, mode, >=0?positionoutput]
  EW_FINDCLOSE,         // FindClose: 1 [handle]
  
  // 60
  EW_FINDNEXT,          // FindNext: 2  [output, handle]
  EW_FINDFIRST,         // FindFirst: 2 [filespec, output, handleoutput]
  EW_WRITEUNINSTALLER,  // WriteUninstaller: 3 [name, offset, icon_size]
  EW_LOG,               // LogText: 2 [0, text] / LogSet: [1, logstate]
  EW_SECTIONSET,        // SectionSetText:    3: [idx, 0, text]
                        // SectionGetText:    3: [idx, 1, output]
                        // SectionSetFlags:   3: [idx, 2, flags]
                        // SectionGetFlags:   3: [idx, 3, output]
  EW_INSTTYPESET,       // InstTypeSetFlags:  3: [idx, 0, flags]
                        // InstTypeGetFlags:  3: [idx, 1, output]
  // instructions not actually implemented in exehead, but used in compiler.
  EW_GETLABELADDR,      // both of these get converted to EW_ASSIGNVAR
  EW_GETFUNCTIONADDR,

  EW_LOCKWINDOW
};

#ifdef NSIS_SCRIPT
static CCommandPair kCommandPairs[] = 
{
  { 0, "Invalid" },
  { 0, "Return" },
  { 1, "Goto" },
  { 0, "Abort" },
  { 0, "Quit" },
  { 1, "Call" },
  { 2, "UpdateSatusText" },
  { 1, "Sleep" },
  { 0, "BringToFront" },
  { 2, "SetDetailsView" },

  { 2, "SetFileAttributes" },
  { 2, "SetOutPath" },
  { 3, "IfFileExists" },
  { 2, "SetFlag" },
  { 4, "IfFlag" },
  { 2, "GetFlag" },
  { 3, "Rename" },
  { 2, "GetFullPathName" },
  { 2, "SearchPath" },
  { 2, "GetTempFileName" },

  { 6, "File" },
  { 2, "Delete" },
  { 5, "MessageBox" },
  { 2, "RMDir" },
  { 2, "Assign" },
  { 4, "StrCpy" },
  { 5, "StrCmp" },
  { 3, "ReadEnvStr" },
  { 6, "IntCmp" },
  { 4, "IntOp" },

  { 3, "IntFmt" },
  { 3, "PushPop" },
  { 5, "FindWindow" },
  { 6, "SendMessage" },
  { 3, "IsWindow" },
  { 3, "GetDlgItem" },
  { 3, "SerCtlColors" },
  { 1, "SetBrandingImage" },
  { 5, "CreateFont" },
  { 2, "ShowWindow" },

  { 4, "ShellExecute" },
  { 3, "Execute" },
  { 3, "GetFileTime" },
  { 3, "GetDLLVersion" },
  { 3, "RegisterDLL" },
  { 5, "CreateShortCut" },
  { 3, "CopyFiles" },
  { 0, "Reboot" },
  { 4, "WriteINIStr" },
  { 4, "ReadINIStr" },

  { 4, "DelReg" },
  { 5, "WriteReg" },
  { 5, "ReadRegStr" },
  { 5, "RegEnum" },
  { 1, "FileClose" },
  { 4, "FileOpen" },
  { 3, "FileWrite" },
  { 4, "FileRead" },
  { 4, "FileSeek" },
  { 1, "FindClose" },

  { 2, "FindNext" },
  { 2, "FindFirst" },
  { 3, "WriteUninstaller" },
  { 2, "LogText" },
  { 3, "Section?etText" },
  { 3, "InstType?etFlags" },
  { 6, "GetLabelAddr" },
  { 2, "GetFunctionAddress" },
  { 6, "LockWindow" }
};

#endif

static const char *kShellStrings[] = 
{
  "",
  "",

  "SMPROGRAMS",
  "",
  "PRINTERS",
  "DOCUMENTS",
  "FAVORITES",
  "SMSTARTUP",
  "RECENT",
  "SENDTO",
  "",
  "STARTMENU",
  "",
  "MUSIC",
  "VIDEO",
  "",

  "DESKTOP",
  "",
  "",
  "NETHOOD",
  "FONTS",
  "TEMPLATES",
  "COMMONSTARTMENU",
  "COMMONFILES",
  "COMMON_STARTUP",
  "COMMON_DESKTOPDIRECTORY",
  "QUICKLAUNCH",
  "PRINTHOOD",
  "LOCALAPPDATA",
  "ALTSTARTUP",
  "ALTSTARTUP",
  "FAVORITES",

  "INTERNET_CACHE",
  "COOKIES",
  "HISTORY",
  "APPDATA",
  "WINDIR",
  "SYSDIR",
  "PROGRAMFILES",
  "PICTURES",
  "PROFILE",
  "",
  "",
  "COMMONFILES",
  "",
  "TEMPLATES",
  "DOCUMENTS",
  "ADMINTOOLS",

  "ADMINTOOLS",
  "",
  "",
  "",
  "",
  "MUSIC",
  "PICTURES",
  "VIDEO",
  "RESOURCES",
  "RESOURCES_LOCALIZED",
  "",
  "CDBURN_AREA"
};

static const int kNumShellStrings = sizeof(kShellStrings) / sizeof(kShellStrings[0]);

/*
# define CMDLINE 20 // everything before here doesn't have trailing slash removal
# define INSTDIR 21
# define OUTDIR 22
# define EXEDIR 23
# define LANGUAGE 24
# define TEMP   25
# define PLUGINSDIR 26
# define HWNDPARENT 27
# define _CLICK 28
# define _OUTDIR 29
*/

static const char *kVarStrings[] = 
{
  "CMDLINE",
  "INSTDIR",
  "OUTDIR",
  "EXEDIR",
  "LANGUAGE",
  "TEMP",
  "PLUGINSDIR",
  "HWNDPARENT",
  "_CLICK",
  "_OUTDIR"
};

static const int kNumVarStrings = sizeof(kVarStrings) / sizeof(kVarStrings[0]);


static AString UIntToString(UInt32 v)
{
  char sz[32];
  ConvertUInt64ToString(v, sz);
  return sz;
}

static AString IntToString(Int32 v)
{
  char sz[32];
  ConvertInt64ToString(v, sz);
  return sz;
}

static AString GetVar(UInt32 index)
{
  AString res = "$";
  if (index < 10)
    res += UIntToString(index);
  else if (index < 20)
  {
    res += "R";
    res += UIntToString(index - 10);
  }
  else if (index < 20 + kNumVarStrings)
    res += kVarStrings[index - 20];
  else
  {
    res += "[";
    res += UIntToString(index);
    res += "]";
  }
  return res;
}

// $0..$9, $INSTDIR, etc are encoded as ASCII bytes starting from this value.
#define NS_SKIP_CODE  252
#define NS_VAR_CODE   253
#define NS_SHELL_CODE 254
#define NS_LANG_CODE  255
#define NS_CODES_START NS_SKIP_CODE

// Based on Dave Laundon's simplified process_string
AString GetNsisString(const AString &s)
{
  AString res;
  for (int i = 0; i < s.Length();)
  {
    unsigned char nVarIdx = s[i++];
    if (nVarIdx > NS_CODES_START && i + 2 <= s.Length())
    {
      int nData = s[i++] & 0x7F;
      unsigned char c1 = s[i++];
      nData |= (((int)(c1 & 0x7F)) << 7);

      if (nVarIdx == NS_SHELL_CODE)
      {
        UInt32 index = c1;
        bool needPrint = true;
        res += "$";
        if (index < kNumShellStrings)
        {
          const char *sz = kShellStrings[index];
          if (sz[0] != 0)
          {
            res += sz;
            needPrint = false;
          }
        }
        if (needPrint)
        {
          res += "SHELL[";
          res += UIntToString(index);
          res += "]";
        }
      }
      else if (nVarIdx == NS_VAR_CODE)
        res += GetVar(nData);
      else if (nVarIdx == NS_LANG_CODE)
        res += "NS_LANG_CODE";
    }
    else if (nVarIdx == NS_SKIP_CODE)
    {
      if (i < s.Length())
        res += s[i++];
    }
    else // Normal char
      res += (char)nVarIdx;
  }
  return res;
}

AString CInArchive::ReadString2(UInt32 pos)
{
  return GetNsisString(ReadString(pos));
}

#define DEL_DIR 1
#define DEL_RECURSE 2
#define DEL_REBOOT 4
// #define DEL_SIMPLE 8

static const int kNumEntryParams = 6;

struct CEntry
{
  UInt32 Which;
  UInt32 Params[kNumEntryParams];
  AString GetParamsString(int numParams);
  CEntry()
  {
    Which = 0;
    for (UInt32 j = 0; j < kNumEntryParams; j++)
      Params[j] = 0;
  }
};

AString CEntry::GetParamsString(int numParams)
{
  AString s;
  for (int i = 0; i < numParams; i++)
  {
    s += " ";
    UInt32 v = Params[i];
    if (v > 0xFFF00000)
      s += IntToString((Int32)Params[i]);
    else
      s += UIntToString(Params[i]);
  }
  return s;
}

HRESULT CInArchive::ReadEntries(const CBlockHeader &bh)
{
  _posInData = bh.Offset + GetOffset();
  AString prefix;
  for (UInt32 i = 0; i < bh.Num; i++)
  {
    CEntry e;
    e.Which = ReadUInt32();
    for (UInt32 j = 0; j < kNumEntryParams; j++)
      e.Params[j] = ReadUInt32();
    #ifdef NSIS_SCRIPT
    if (e.Which != EW_PUSHPOP && e.Which < sizeof(kCommandPairs) / sizeof(kCommandPairs[0]))
    {
      const CCommandPair &pair = kCommandPairs[e.Which];
      Script += pair.Name;
    }
    #endif

    switch (e.Which)
    {
      case EW_CREATEDIR:
      {
        prefix.Empty();
        prefix = ReadString2(e.Params[0]);
        #ifdef NSIS_SCRIPT
        Script += " ";
        Script += prefix;
        #endif
        break;
      }

      case EW_EXTRACTFILE:
      {
        CItem item;
        item.Prefix = prefix;
        /* UInt32 overwriteFlag = e.Params[0]; */
        item.Name = ReadString2(e.Params[1]);
        item.Pos = e.Params[2];
        item.DateTime.dwLowDateTime = e.Params[3];
        item.DateTime.dwHighDateTime = e.Params[4];
        /* UInt32 allowIgnore = e.Params[5]; */
        if (Items.Size() > 0)
        {
          /*
          if (item.Pos == Items.Back().Pos)
            continue;
          */
        }
        Items.Add(item);
        #ifdef NSIS_SCRIPT
        Script += " ";
        Script += item.Name;
        #endif
        break;
      }


      #ifdef NSIS_SCRIPT
      case EW_UPDATETEXT:
      {
        Script += " ";
        Script += ReadString2(e.Params[0]);
        Script += " ";
        Script += UIntToString(e.Params[1]);
        break;
      }
      case EW_SETFILEATTRIBUTES:
      {
        Script += " ";
        Script += ReadString2(e.Params[0]);
        Script += " ";
        Script += UIntToString(e.Params[1]);
        break;
      }
      case EW_IFFILEEXISTS:
      {
        Script += " ";
        Script += ReadString2(e.Params[0]);
        Script += " ";
        Script += UIntToString(e.Params[1]);
        Script += " ";
        Script += UIntToString(e.Params[2]);
        break;
      }
      case EW_RENAME:
      {
        Script += " ";
        Script += ReadString2(e.Params[0]);
        Script += " ";
        Script += ReadString2(e.Params[1]);
        Script += " ";
        Script += UIntToString(e.Params[2]);
        break;
      }
      case EW_GETFULLPATHNAME:
      {
        Script += " ";
        Script += ReadString2(e.Params[0]);
        Script += " ";
        Script += ReadString2(e.Params[1]);
        Script += " ";
        Script += UIntToString(e.Params[2]);
        break;
      }
      case EW_SEARCHPATH:
      {
        Script += " ";
        Script += ReadString2(e.Params[0]);
        Script += " ";
        Script += ReadString2(e.Params[1]);
        break;
      }
      case EW_GETTEMPFILENAME:
      {
        AString s;
        Script += " ";
        Script += ReadString2(e.Params[0]);
        Script += " ";
        Script += ReadString2(e.Params[1]);
        break;
      }

      case EW_DELETEFILE:
      {
        UInt64 flag = e.Params[1];
        if (flag != 0)
        {
          Script += " ";
          if (flag == DEL_REBOOT)
            Script += "/REBOOTOK";
          else
            Script += UIntToString(e.Params[1]);
        }
        Script += " ";
        Script += ReadString2(e.Params[0]);
        break;
      }
      case EW_RMDIR:
      {
        UInt64 flag = e.Params[1];
        if (flag != 0)
        {
          if ((flag & DEL_REBOOT) != 0)
            Script += " /REBOOTOK";
          if ((flag & DEL_RECURSE) != 0)
            Script += " /r";
        }
        Script += " ";
        Script += ReadString2(e.Params[0]);
        break;
      }
      case EW_ASSIGNVAR:
      {
        Script += " ";
        Script += GetVar(e.Params[0]);;
        Script += " \"";
        AString maxLen, startOffset;
        Script += ReadString2(e.Params[1]);
        Script += "\"";
        if (e.Params[2] != 0)
          maxLen = ReadString(e.Params[2]);
        if (e.Params[3] != 0)
          startOffset = ReadString(e.Params[3]);
        if (!maxLen.IsEmpty() || !startOffset.IsEmpty())
        {
          Script += " ";
          if (maxLen.IsEmpty())
            Script += "\"\"";
          else
            Script += maxLen;
          if (!startOffset.IsEmpty())
          {
            Script += " ";
            Script += startOffset;
          }
        }
        break;
      }
      case EW_STRCMP:
      {
        Script += " ";

        Script += " \"";
        Script += ReadString2(e.Params[0]);
        Script += "\"";
        
        Script += " \"";
        Script += ReadString2(e.Params[1]);
        Script += "\"";

        for (int j = 2; j < 5; j++)
        {
          Script += " ";
          Script += UIntToString(e.Params[j]);
        }
        break;
      }

      case EW_PUSHPOP:
      {
        int isPop = (e.Params[1] != 0);
        if (isPop)
        {
          Script += "Pop";
          Script += " ";
          Script += GetVar(e.Params[0]);;
        }
        else
        {
          int isExch = (e.Params[2] != 0);
          if (isExch)
          {
            Script += "Exch";
          }
          else
          {
            Script += "Push";
            Script += " ";
            Script += ReadString2(e.Params[0]);
          }
        }
        break;
      }

      /*
      case EW_SENDMESSAGE:
      {
        Script += " ";
        Script += IntToString(e.Params[0]);
        Script += " ";
        Script += GetVar(e.Params[1]);
        Script += " ";
        Script += ReadString2(e.Params[2]);
        Script += " ";
        Script += UIntToString(e.Params[3]);
        Script += " ";
        Script += IntToString(e.Params[4]);
        Script += " ";
        Script += UIntToString(e.Params[5]);
        break;
      }
      */

      case EW_REGISTERDLL:
      {
        Script += " ";
        Script += ReadString2(e.Params[0]);
        Script += " ";
        Script += ReadString2(e.Params[1]);
        Script += " ";
        Script += UIntToString(e.Params[2]);
        break;
      }

      case EW_CREATESHORTCUT:
      {
        AString s;

        Script += " ";
        Script += " \"";
        Script += ReadString2(e.Params[0]);
        Script += " \"";

        Script += " ";
        Script += " \"";
        Script += ReadString2(e.Params[1]);
        Script += " \"";

        for (int j = 2; j < 5; j++)
        {
          Script += " ";
          Script += UIntToString(e.Params[j]);
        }
        break;
      }

      /*
      case EW_DELREG:
      {
        AString keyName, valueName;
        keyName = ReadString2(e.Params[1]);
        bool isValue = (e.Params[2] != -1);
        if (isValue)
        {
          valueName = ReadString2(e.Params[2]);
          Script += "Key";
        }
        else
          Script += "Value";
        Script += " ";
        Script += UIntToString(e.Params[0]);
        Script += " ";
        Script += keyName;
        if (isValue)
        {
          Script += " ";
          Script += valueName;
        }
        Script += " ";
        Script += UIntToString(e.Params[3]);
        break;
      }
      */

      case EW_WRITEUNINSTALLER:
      {
        Script += " ";
        Script += ReadString2(e.Params[0]);
        for (int j = 1; j < 3; j++)
        {
          Script += " ";
          Script += UIntToString(e.Params[j]);
        }
        break;
      }

      default:
      {
        int numParams = kNumEntryParams;
        if (e.Which < sizeof(kCommandPairs) / sizeof(kCommandPairs[0]))
        {
          const CCommandPair &pair = kCommandPairs[e.Which];
          // Script += pair.Name;
          numParams = pair.NumParams;
        }
        else
        {
          Script += "Unknown";
          Script += UIntToString(e.Which);
        }
        Script += e.GetParamsString(numParams);
      }
      #endif
    }
    #ifdef NSIS_SCRIPT
    Script += kCrLf;
    #endif
  }

  {
    Items.Sort(CompareItems, 0);
    int i;
    // if (IsSolid) 
    for (i = 0; i + 1 < Items.Size();)
    {
      if (Items[i].Pos == Items[i + 1].Pos && (IsSolid || Items[i].Name == Items[i + 1].Name))
        Items.Delete(i + 1);
      else
        i++;
    }
    for (i = 0; i + 1 < Items.Size(); i++)
    {
      CItem &item = Items[i];
      item.EstimatedSizeIsDefined = true;
      item.EstimatedSize = Items[i + 1].Pos - item.Pos - 4;
    }
    if (!IsSolid)
    {
      for (i = 0; i < Items.Size(); i++)
      {
        CItem &item = Items[i];
        RINOK(_stream->Seek(GetPosOfNonSolidItem(i), STREAM_SEEK_SET, NULL));
        const UInt32 kSigSize = 4 + 1 + 5;
        BYTE sig[kSigSize];
        UInt32 processedSize;
        RINOK(ReadStream(_stream, sig, kSigSize, &processedSize));
        if (processedSize < 4)
          return S_FALSE;
        UInt32 size = GetUInt32FromMemLE(sig);
        if ((size & 0x80000000) != 0)
        {
          item.IsCompressed = true;
          // is compressed;
          size &= ~0x80000000;
          if (Method == NMethodType::kLZMA)
          {
            if (processedSize < 9)
              return S_FALSE;
            if (FilterFlag)
              item.UseFilter = (sig[4] != 0);
            item.DictionarySize = GetUInt32FromMemLE(sig + 5 + (FilterFlag ? 1 : 0));
          }
        }
        else
        {
          item.IsCompressed = false;
          item.Size = size;
          item.SizeIsDefined = true;
        }
        item.CompressedSize = size;
        item.CompressedSizeIsDefined = true;
      }
    }
  }
  return S_OK;
}

HRESULT CInArchive::Parse()
{
  // UInt32 offset = ReadUInt32();
  // ???? offset == FirstHeader.HeaderLength
  /* UInt32 ehFlags = */ ReadUInt32();
  CBlockHeader bhPages, bhSections, bhEntries, bhStrings, bhLangTables, bhCtlColors, bhData;
  // CBlockHeader bgFont;
  ReadBlockHeader(bhPages);
  ReadBlockHeader(bhSections);
  ReadBlockHeader(bhEntries);
  ReadBlockHeader(bhStrings);
  ReadBlockHeader(bhLangTables);
  ReadBlockHeader(bhCtlColors);
  // ReadBlockHeader(bgFont);
  ReadBlockHeader(bhData);

  _stringsPos = bhStrings.Offset;

  return ReadEntries(bhEntries);
}

static bool IsLZMA(const Byte *p, UInt32 &dictionary)
{
  dictionary = GetUInt32FromMemLE(p + 1);
  return (p[0] == 0x5D && p[1] == 0x00 && p[2] == 0x00 && p[5] == 0x00);
}

static bool IsLZMA(const Byte *p, UInt32 &dictionary, bool &thereIsFlag)
{
  if (IsLZMA(p, dictionary))
  {
    thereIsFlag = false;
    return true;
  }
  if (IsLZMA(p + 1, dictionary))
  {
    thereIsFlag = true;
    return true;
  }
  return false;
}

HRESULT CInArchive::Open2(
      DECL_EXTERNAL_CODECS_LOC_VARS2
      )
{
  RINOK(_stream->Seek(0, STREAM_SEEK_CUR, &StreamOffset));

  const UInt32 kSigSize = 4 + 1 + 5 + 1; // size, flag, lzma props, lzma first byte
  BYTE sig[kSigSize];
  UInt32 processedSize;
  RINOK(ReadStream(_stream, sig, kSigSize, &processedSize));
  if (processedSize != kSigSize)
    return S_FALSE;
  UInt64 position;
  RINOK(_stream->Seek(StreamOffset, STREAM_SEEK_SET, &position));

  _headerIsCompressed = true;
  IsSolid = true;
  FilterFlag = false;

  UInt32 compressedHeaderSize = GetUInt32FromMemLE(sig);
  
  if (compressedHeaderSize == FirstHeader.HeaderLength)
  {
    _headerIsCompressed = false;
    IsSolid = false;
    Method = NMethodType::kCopy;
  }
  else if (IsLZMA(sig, DictionarySize, FilterFlag))
  {
    Method = NMethodType::kLZMA;
  }
  else if (IsLZMA(sig + 4, DictionarySize, FilterFlag))
  {
    IsSolid = false;
    Method = NMethodType::kLZMA;
  }
  else if (sig[3] == 0x80)
  {
    IsSolid = false;
    Method = NMethodType::kDeflate;
  }
  else
  {
    Method = NMethodType::kDeflate;
  }

  _posInData = 0;
  if (!IsSolid)
  {
    _headerIsCompressed = ((compressedHeaderSize & 0x80000000) != 0);
    if (_headerIsCompressed)
      compressedHeaderSize &= ~0x80000000;
    _nonSolidStartOffset = compressedHeaderSize;
    RINOK(_stream->Seek(StreamOffset + 4, STREAM_SEEK_SET, NULL));
  }
  UInt32 unpackSize = FirstHeader.HeaderLength;
  if (_headerIsCompressed)
  {
    // unpackSize = (1 << 23);
    _data.SetCapacity(unpackSize);
    RINOK(Decoder.Init(
        EXTERNAL_CODECS_LOC_VARS
        _stream, Method, FilterFlag, UseFilter));
    UInt32 processedSize;
    RINOK(Decoder.Read(_data, unpackSize, &processedSize));
    if (processedSize != unpackSize)
      return S_FALSE;
    _size = (size_t)processedSize;
    if (IsSolid)
    {
      UInt32 size2 = ReadUInt32();
      if (size2 < _size)
        _size = size2;
    }
  }
  else
  {
    _data.SetCapacity(unpackSize);
    _size = (size_t)unpackSize;
    RINOK(ReadStream(_stream, (Byte *)_data, unpackSize, &processedSize));
    if (processedSize != unpackSize)
      return S_FALSE;
  }
  return Parse();
}

/*
NsisExe = 
{
  ExeStub
  Archive  // must start from 512 * N
  #ifndef NSIS_CONFIG_CRC_ANAL
  {
    Some additional data
  }
}

Archive
{
  FirstHeader
  Data
  #ifdef NSIS_CONFIG_CRC_SUPPORT && FirstHeader.ThereIsCrc()
  {
    CRC
  }
}

FirstHeader
{
  UInt32 Flags;
  Byte Signature[16];
  // points to the header+sections+entries+stringtable in the datablock
  UInt32 HeaderLength;
  UInt32 ArchiveSize;
}
*/

HRESULT CInArchive::Open(
    DECL_EXTERNAL_CODECS_LOC_VARS
    IInStream *inStream, const UInt64 *maxCheckStartPosition)
{
  Clear();
  UInt64 pos;
  RINOK(inStream->Seek(0, STREAM_SEEK_CUR, &pos));
  RINOK(inStream->Seek(0, STREAM_SEEK_END, &_archiveSize));
  UInt64 position;
  RINOK(inStream->Seek(pos, STREAM_SEEK_SET, &position));
  UInt64 maxSize = (maxCheckStartPosition != 0) ? *maxCheckStartPosition : (1 << 20);
  const UInt32 kStep = 512;
  const UInt32 kStartHeaderSize = 4 * 7;
  Byte buffer[kStep];
  bool found = false;
  
  UInt64 headerPosition = 0;
  while (position <= maxSize)
  {
    UInt32 processedSize;
    RINOK(ReadStream(inStream, buffer, kStartHeaderSize, &processedSize));
    if (processedSize != kStartHeaderSize)
      return S_FALSE;
    headerPosition = position;
    position += processedSize;
    if(memcmp(buffer + 4, kSignature, kSignatureSize) == 0)
    {
      found = true;
      break;
    }
    const UInt32 kRem = kStep - kStartHeaderSize;
    RINOK(ReadStream(inStream, buffer + kStartHeaderSize, kRem, &processedSize));
    if (processedSize != kRem)
      return S_FALSE;
    position += processedSize;
  }
  if (!found)
    return S_FALSE;
  FirstHeader.Flags = GetUInt32FromMemLE(buffer);
  FirstHeader.HeaderLength = GetUInt32FromMemLE(buffer + kSignatureSize + 4);
  FirstHeader.ArchiveSize = GetUInt32FromMemLE(buffer + kSignatureSize + 8);
  if (_archiveSize - headerPosition < FirstHeader.ArchiveSize)
    return S_FALSE;

  _stream = inStream;
  HRESULT res = S_FALSE;
  try 
  { 
    res = Open2(
      EXTERNAL_CODECS_LOC_VARS2
      ); 
  }
  catch(...) { Clear(); res = S_FALSE; }
  _stream.Release();
  return res;
}

void CInArchive::Clear()
{
  #ifdef NSIS_SCRIPT
  Script.Empty();
  #endif
  Items.Clear();
}

}}
