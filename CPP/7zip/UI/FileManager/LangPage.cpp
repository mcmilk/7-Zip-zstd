// LangPage.cpp

#include "StdAfx.h"

#include "../../../Common/Lang.h"

#include "../../../Windows/FileFind.h"
#include "../../../Windows/ResourceString.h"

#include "HelpUtils.h"
#include "LangPage.h"
#include "LangPageRes.h"
#include "LangUtils.h"
#include "RegistryUtils.h"

using namespace NWindows;


static const unsigned k_NumLangLines_EN = 443;

#ifdef Z7_LANG
static const UInt32 kLangIDs[] =
{
  IDT_LANG_LANG
};
#endif

#define kLangTopic "fm/options.htm#language"


struct CLangListRecord
{
  int Order;
  unsigned LangInfoIndex;
  bool IsSelected;
  UString Mark;
  UString Name;

  CLangListRecord(): Order (10), IsSelected(false) {}
  int Compare(const CLangListRecord &a) const
  {
    if (Order < a.Order) return -1;
    if (Order > a.Order) return 1;
    return MyStringCompareNoCase(Name, a.Name);
  }
};


static void NativeLangString(UString &dest, const wchar_t *s)
{
  dest += " (";
  dest += s;
  dest.Add_Char(')');
}

bool LangOpen(CLang &lang, CFSTR fileName);

bool CLangPage::OnInit()
{
#ifdef Z7_LANG
  LangSetDlgItems(*this, kLangIDs, Z7_ARRAY_SIZE(kLangIDs));
#endif
  _langCombo.Attach(GetItem(IDC_LANG_LANG));


  unsigned listRecords_SelectedIndex = 0;

  CObjectVector<CLangListRecord> listRecords;
  {
    CLangListRecord listRecord;
    listRecord.Order = 0;
    listRecord.Mark = "---";
    listRecord.Name = MyLoadString(IDS_LANG_ENGLISH);
    NativeLangString(listRecord.Name, MyLoadString(IDS_LANG_NATIVE));
    listRecord.LangInfoIndex = _langs.Size();
    listRecords.Add(listRecord);
  }

  AStringVector names;
  unsigned subLangIndex = 0;
  Lang_GetShortNames_for_DefaultLang(names, subLangIndex);

  const FString dirPrefix = GetLangDirPrefix();
  NFile::NFind::CEnumerator enumerator;
  enumerator.SetDirPrefix(dirPrefix);
  NFile::NFind::CFileInfo fi;

  CLang lang_en;
  {
    CLangInfo &langInfo = _langs.AddNew();
    langInfo.Name = "-";
    if (LangOpen(lang_en, dirPrefix + FTEXT("en.ttt")))
    {
      langInfo.NumLines = lang_en._ids.Size();
      // langInfo.Comments = lang_en.Comments;
    }
    else
      langInfo.NumLines = k_NumLangLines_EN;
    NumLangLines_EN = langInfo.NumLines;
  }

  CLang lang;
  UString error;
  UString n;
  
  while (enumerator.Next(fi))
  {
    if (fi.IsDir())
      continue;
    const unsigned kExtSize = 4;
    if (fi.Name.Len() < kExtSize)
      continue;
    const unsigned pos = fi.Name.Len() - kExtSize;
    if (!StringsAreEqualNoCase_Ascii(fi.Name.Ptr(pos), ".txt"))
    {
      // if (!StringsAreEqualNoCase_Ascii(fi.Name.Ptr(pos), ".ttt"))
      continue;
    }

    if (!LangOpen(lang, dirPrefix + fi.Name))
    {
      error.Add_Space_if_NotEmpty();
      error += fs2us(fi.Name);
      continue;
    }
    
    const UString shortName = fs2us(fi.Name.Left(pos));

    CLangListRecord listRecord;
    if (!names.IsEmpty())
    {
      for (unsigned i = 0; i < names.Size(); i++)
        if (shortName.IsEqualTo_Ascii_NoCase(names[i]))
        {
          if (subLangIndex == i || names.Size() == 1)
          {
            listRecord.Mark = "***";
            // listRecord.Order = 1;
          }
          else
          {
            listRecord.Mark = "+++";
            // listRecord.Order = 2;
          }
          break;
        }
      if (listRecord.Mark.IsEmpty())
      {
        const int minusPos = shortName.Find(L'-');
        if (minusPos >= 0)
        {
          const UString shortName2 = shortName.Left(minusPos);
          if (shortName2.IsEqualTo_Ascii_NoCase(names[0]))
          {
            listRecord.Mark = "+++";
            // listRecord.Order = 3;
          }
        }
      }
    }
    UString s = shortName;
    const wchar_t *eng = lang.Get(IDS_LANG_ENGLISH);
    if (eng)
      s = eng;
    const wchar_t *native = lang.Get(IDS_LANG_NATIVE);
    if (native)
      NativeLangString(s, native);
    
    listRecord.Name = s;
    listRecord.LangInfoIndex = _langs.Size();
    listRecords.Add(listRecord);
    if (g_LangID.IsEqualTo_NoCase(shortName))
      listRecords_SelectedIndex = listRecords.Size() - 1;

    CLangInfo &langInfo = _langs.AddNew();
    langInfo.Comments = lang.Comments;
    langInfo.Name = shortName;
    unsigned numLines = lang._ids.Size();
    if (!lang_en.IsEmpty())
    {
      numLines = 0;
      unsigned i1 = 0;
      unsigned i2 = 0;
      for (;;)
      {
        UInt32 id1 = (UInt32)0 - 1;
        UInt32 id2 = (UInt32)0 - 1;
        bool id1_defined = false;
        bool id2_defined = false;
        if (i1 < lang_en._ids.Size())
        {
          id1 = lang_en._ids[i1];
          id1_defined = true;
        }
        if (i2 < lang._ids.Size())
        {
          id2 = lang._ids[i2];
          id2_defined = true;
        }
        
        bool id1_is_smaller = true;
        if (id1_defined)
        {
          if (id2_defined)
          {
            if (id1 == id2)
            {
              i1++;
              i2++;
              numLines++;
              continue;
            }
            if (id1 > id2)
              id1_is_smaller = false;
          }
        }
        else if (!id2_defined)
          break;
        else
          id1_is_smaller = false;

        n.Empty();
        if (id1_is_smaller)
        {
          n.Add_UInt32(id1);
          n += " : ";
          n += lang_en.Get_by_index(i1);
          langInfo.MissingLines.Add(n);
          i1++;
        }
        else
        {
          n.Add_UInt32(id2);
          n += " : ";
          n += lang.Get_by_index(i2);
          langInfo.ExtraLines.Add(n);
          i2++;
        }
      }
    }
    langInfo.NumLines = numLines + langInfo.ExtraLines.Size();
  }

  listRecords[listRecords_SelectedIndex].IsSelected = true;

  listRecords.Sort();
  FOR_VECTOR (i, listRecords)
  {
    const CLangListRecord &rec= listRecords[i];
    UString temp = rec.Name;
    if (!rec.Mark.IsEmpty())
    {
      temp += "  ";
      temp += rec.Mark;
    }
    const int index = (int)_langCombo.AddString(temp);
    _langCombo.SetItemData(index, (LPARAM)rec.LangInfoIndex);
    if (rec.IsSelected)
      _langCombo.SetCurSel(index);
  }

  ShowLangInfo();
  
  if (!error.IsEmpty())
    MessageBoxW(NULL, error, L"Error in Lang file", MB_ICONERROR);
  return CPropertyPage::OnInit();
}

LONG CLangPage::OnApply()
{
  if (_needSave)
  {
    const int pathIndex = (int)_langCombo.GetItemData_of_CurSel();
    if ((unsigned)pathIndex < _langs.Size())
      SaveRegLang(_langs[pathIndex].Name);
  }
  _needSave = false;
  #ifdef Z7_LANG
  ReloadLang();
  #endif
  LangWasChanged = true;
  return PSNRET_NOERROR;
}

void CLangPage::OnNotifyHelp()
{
  ShowHelpWindow(kLangTopic);
}

bool CLangPage::OnCommand(unsigned code, unsigned itemID, LPARAM param)
{
  if (code == CBN_SELCHANGE && itemID == IDC_LANG_LANG)
  {
    _needSave = true;
    Changed();
    ShowLangInfo();
    return true;
  }
  return CPropertyPage::OnCommand(code, itemID, param);
}

static void AddVectorToString(UString &s, const UStringVector &v)
{
  UString a;
  FOR_VECTOR (i, v)
  {
    if (i >= 50)
      break;
    a = v[i];
    if (a.Len() > 1500)
      continue;
    if (a[0] == ';')
    {
      a.DeleteFrontal(1);
      a.Trim();
    }
    s += a;
    s.Add_LF();
  }
}

static void AddVectorToString2(UString &s, const char *name, const UStringVector &v)
{
  if (v.IsEmpty())
    return;
  s.Add_LF();
  s += "------ ";
  s += name;
  s += ": ";
  s.Add_UInt32(v.Size());
  s += " :";
  s.Add_LF();
  AddVectorToString(s, v);
}

void CLangPage::ShowLangInfo()
{
  UString s;
  const int pathIndex = (int)_langCombo.GetItemData_of_CurSel();
  if ((unsigned)pathIndex < _langs.Size())
  {
    const CLangInfo &langInfo = _langs[pathIndex];
    const unsigned numLines = langInfo.NumLines;
    s += langInfo.Name;
    s += " : ";
    s.Add_UInt32(numLines);
    if (NumLangLines_EN != 0)
    {
      s += " / ";
      s.Add_UInt32(NumLangLines_EN);
      s += " = ";
      s.Add_UInt32(numLines * 100 / NumLangLines_EN);
      s += "%";
    }
    s.Add_LF();
    AddVectorToString(s, langInfo.Comments);
    AddVectorToString2(s, "Missing lines", langInfo.MissingLines);
    AddVectorToString2(s, "Extra lines", langInfo.ExtraLines);
  }
  SetItemText(IDT_LANG_INFO, s);
}
