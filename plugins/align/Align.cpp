#include <CRT/crt.hpp>
#include <plugin.hpp>
#include <PluginSettings.hpp>
#include <DlgBuilder.hpp>
#include "AlignLng.hpp"
#include "version.hpp"
#include <initguid.h>
#include "guid.hpp"

#if defined(__GNUC__)

#ifdef __cplusplus
extern "C"{
#endif
  BOOL WINAPI DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved);
#ifdef __cplusplus
};
#endif

BOOL WINAPI DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved)
{
  (void) lpReserved;
  (void) dwReason;
  (void) hDll;
  return TRUE;
}
#endif

static struct PluginStartupInfo Info;
static struct FarStandardFunctions FSF;

static void ReformatBlock(int RightMargin,int SmartMode,int Justify);
static void JustifyBlock(int RightMargin);
static int JustifyString(int RightMargin,struct EditorSetString &ess);

const wchar_t *GetMsg(int MsgId)
{
  return Info.GetMsg(&MainGuid,MsgId);
}

void WINAPI GetGlobalInfoW(struct GlobalInfo *Info)
{
  Info->StructSize=sizeof(GlobalInfo);
  Info->MinFarVersion=FARMANAGERVERSION;
  Info->Version=PLUGIN_VERSION;
  Info->Guid=MainGuid;
  Info->Title=PLUGIN_NAME;
  Info->Description=PLUGIN_DESC;
  Info->Author=PLUGIN_AUTHOR;
}


void WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info)
{
  ::Info=*Info;
  ::FSF=*Info->FSF;
  ::Info.FSF=&::FSF;
}

void WINAPI GetPluginInfoW(struct PluginInfo *Info)
{
  Info->StructSize=sizeof(*Info);
  Info->Flags=PF_EDITOR|PF_DISABLEPANELS;
  static const wchar_t *PluginMenuStrings[1];
  PluginMenuStrings[0]=GetMsg(MAlign);
  Info->PluginMenu.Guids=&MenuGuid;
  Info->PluginMenu.Strings=PluginMenuStrings;
  Info->PluginMenu.Count=ARRAYSIZE(PluginMenuStrings);
}


HANDLE WINAPI OpenW(const struct OpenInfo *OInfo)
{
  PluginSettings settings(MainGuid, Info.SettingsControl);

  int RightMargin=settings.Get(0,L"RightMargin",75);
  int Reformat=settings.Get(0,L"Reformat",TRUE);
  int SmartMode=settings.Get(0,L"SmartMode",FALSE);
  int Justify=settings.Get(0,L"Justify",FALSE);

  PluginDialogBuilder Builder(Info, MainGuid, DialogGuid, MAlign, nullptr);
  FarDialogItem *RightMarginItem = Builder.AddIntEditField(&RightMargin, 3);
  Builder.AddTextAfter(RightMarginItem, MRightMargin);
  Builder.AddCheckbox(MReformat, &Reformat);
  Builder.AddCheckbox(MSmartMode, &SmartMode);
  Builder.AddCheckbox(MJustify, &Justify);
  Builder.AddOKCancel(MOk, MCancel);

  if (Builder.ShowDialog())
  {
    settings.Set(0,L"Reformat",Reformat);
    settings.Set(0,L"RightMargin",RightMargin);
    settings.Set(0,L"SmartMode",SmartMode);
    settings.Set(0,L"Justify",Justify);
    if (Reformat)
      ReformatBlock(RightMargin,SmartMode,Justify);
    else if (Justify)
      JustifyBlock(RightMargin);
  }

  return nullptr;
}


void ReformatBlock(int RightMargin,int SmartMode,int Justify)
{
  EditorInfo ei;
  Info.EditorControl(-1,ECTL_GETINFO,0,&ei);

  if (ei.BlockType!=BTYPE_STREAM || RightMargin<1)
    return;

  struct EditorSetPosition esp;
  memset(&esp,-1,sizeof(esp));
  esp.CurLine=ei.BlockStartLine;
  esp.CurPos=0;
  Info.EditorControl(-1,ECTL_SETPOSITION,0,&esp);

  wchar_t *TotalString=NULL;
  int TotalLength=0,IndentSize=0x7fffffff;

  while (1)
  {
    struct EditorGetString egs;
    egs.StringNumber=-1;
    if (!Info.EditorControl(-1,ECTL_GETSTRING,0,&egs))
      break;
    if (egs.SelStart==-1 || egs.SelStart==egs.SelEnd)
      break;
    int ExpandNum=-1;
    Info.EditorControl(-1,ECTL_EXPANDTABS,0,&ExpandNum);
    Info.EditorControl(-1,ECTL_GETSTRING,0,&egs);

    int SpaceLength=0;
    while (SpaceLength<egs.StringLength && egs.StringText[SpaceLength]==L' ')
      SpaceLength++;

    while (egs.StringLength>0 && *egs.StringText==L' ')
    {
      egs.StringText++;
      egs.StringLength--;
    }

    if (egs.StringLength>0)
    {
      if (SpaceLength<IndentSize)
        IndentSize=SpaceLength;

      TotalString=(wchar_t *)realloc(TotalString,(TotalLength+egs.StringLength+2)*sizeof(wchar_t));
      if (TotalLength!=0 && TotalString[TotalLength-1]!=L' ')
        TotalString[TotalLength++]=L' ';

      wmemcpy(TotalString+TotalLength,egs.StringText,egs.StringLength);
      TotalLength+=egs.StringLength;
    }
    if (!Info.EditorControl(-1,ECTL_DELETESTRING,0,0))
    {
      free(TotalString);
      return;
    }
  }
  if(TotalString)
    TotalString[TotalLength++]=L' ';

  if (IndentSize>=RightMargin)
    IndentSize=RightMargin-1;

  const int MaxIndent=1024;
  if (IndentSize>=MaxIndent)
    IndentSize=MaxIndent-1;

  wchar_t IndentBuf[MaxIndent];
  if (IndentSize>0)
  {
    wmemset(IndentBuf,L' ',IndentSize);
    IndentBuf[IndentSize]=0;
  }

  Info.EditorControl(-1,ECTL_SETPOSITION,0,&esp);
  Info.EditorControl(-1,ECTL_INSERTSTRING,0,0);
  Info.EditorControl(-1,ECTL_SETPOSITION,0,&esp);

  int LastSplitPos=0,PrevSpacePos;
  while (LastSplitPos<TotalLength && TotalString[LastSplitPos]==L' ')
    LastSplitPos++;
  PrevSpacePos=LastSplitPos;

  for (int I=LastSplitPos;I<TotalLength;I++)
  {
    int Length=I-LastSplitPos;
    int LastLength=PrevSpacePos-LastSplitPos;
    if (TotalString[I]==L' ' && Length>RightMargin-IndentSize && LastLength<=RightMargin-IndentSize)
    {
      if (LastLength<=0)
      {
        PrevSpacePos=I;
        LastLength=Length;
      }

      if (SmartMode)
      {
        int Space1=-1,Space2=-1;
        for (int J=PrevSpacePos-1;J>LastSplitPos+20;J--)
        {
          if (TotalString[J]==L' ')
          {
            if (Space2==-1)
              Space2=J;
            else
            {
              if (Space1==-1)
                Space1=J;
              else
                break;
            }
          }
        }
        if (Space2!=-1 && PrevSpacePos-Space2<4)
          if (Space1==-1 || Space2-Space1>4 || PrevSpacePos-Space2==2)
          {
            PrevSpacePos=Space2;
            while (PrevSpacePos>LastSplitPos+1 && TotalString[PrevSpacePos-1]==L' ')
              PrevSpacePos--;
            LastLength=PrevSpacePos-LastSplitPos;
          }
      }

      I=PrevSpacePos;

      struct EditorSetString ess;
      ess.StringNumber=-1;
      ess.StringText=TotalString+LastSplitPos;
      ess.StringEOL=NULL;
      ess.StringLength=LastLength;
      while (ess.StringLength>0 && ess.StringText[ess.StringLength-1]==L' ')
        ess.StringLength--;
      if (!Justify || ess.StringLength>=RightMargin-IndentSize || !JustifyString(RightMargin-IndentSize,ess))
        Info.EditorControl(-1,ECTL_SETSTRING,0,&ess);
      else
        LastLength=RightMargin;

      while (I<TotalLength && TotalString[I]==L' ')
        PrevSpacePos=I++;

      struct EditorSetPosition esp;
      memset(&esp,-1,sizeof(esp));
      esp.CurLine=-1;

      if (IndentSize>0)
      {
        esp.CurPos=0;
        Info.EditorControl(-1,ECTL_SETPOSITION,0,&esp);
        Info.EditorControl(-1,ECTL_INSERTTEXT,0,IndentBuf);
      }

      esp.CurPos=LastLength+IndentSize;
      Info.EditorControl(-1,ECTL_SETPOSITION,0,&esp);
      Info.EditorControl(-1,ECTL_INSERTSTRING,0,0);

      LastSplitPos=I;
    }
    if (TotalString[I]==L' ')
      PrevSpacePos=I;
  }
  struct EditorSetString ess;
  ess.StringNumber=-1;
  ess.StringText=TotalString+LastSplitPos;
  ess.StringEOL=NULL;
  ess.StringLength=TotalLength-LastSplitPos;
  while (ess.StringLength>0 && ess.StringText[ess.StringLength-1]==L' ')
    ess.StringLength--;
  Info.EditorControl(-1,ECTL_SETSTRING,0,&ess);

  if (IndentSize>0)
  {
    struct EditorSetPosition esp;
    memset(&esp,-1,sizeof(esp));
    esp.CurLine=-1;
    esp.CurPos=0;
    Info.EditorControl(-1,ECTL_SETPOSITION,0,&esp);
    Info.EditorControl(-1,ECTL_INSERTTEXT,0,IndentBuf);
  }

  free(TotalString);

  memset(&esp,-1,sizeof(esp));
  esp.CurLine=ei.CurLine;
  esp.CurPos=ei.CurPos;
  Info.EditorControl(-1,ECTL_SETPOSITION,0,&esp);
}


void JustifyBlock(int RightMargin)
{
  EditorInfo ei;
  Info.EditorControl(-1,ECTL_GETINFO,0,&ei);

  if (ei.BlockType!=BTYPE_STREAM)
    return;

  struct EditorGetString egs;
  egs.StringNumber=ei.BlockStartLine;

  while (1)
  {
    if (!Info.EditorControl(-1,ECTL_GETSTRING,0,&egs))
      break;
    if (egs.SelStart==-1 || egs.SelStart==egs.SelEnd)
      break;
    int ExpNum=egs.StringNumber;
    if (!Info.EditorControl(-1,ECTL_EXPANDTABS,0,&ExpNum))
      break;
    Info.EditorControl(-1,ECTL_GETSTRING,0,&egs);

    struct EditorSetString ess;
    ess.StringNumber=egs.StringNumber;

    ess.StringText=(wchar_t*)egs.StringText;
    ess.StringEOL=(wchar_t*)egs.StringEOL;
    ess.StringLength=egs.StringLength;

    if (ess.StringLength<RightMargin)
      JustifyString(RightMargin,ess);

    egs.StringNumber++;
  }
}


int JustifyString(int RightMargin,struct EditorSetString &ess)
{
  int WordCount=0;
  int I;
  for (I=0;I<ess.StringLength-1;I++)
    if (ess.StringText[I]!=L' ' && ess.StringText[I+1]==L' ')
      WordCount++;
  if (ess.StringLength>0 && ess.StringText[ess.StringLength-1]==L' ')
    WordCount--;
  if (WordCount<=0)
    return(FALSE);
  while (ess.StringLength>0 && ess.StringText[ess.StringLength-1]==L' ')
    ess.StringLength--;
  int TotalAddSize=RightMargin-ess.StringLength;
  int AddSize=TotalAddSize/WordCount;
  int Reminder=TotalAddSize%WordCount;

  wchar_t *NewString=(wchar_t *)malloc(RightMargin*sizeof(wchar_t));
  wmemset(NewString,L' ',RightMargin);
  wmemcpy(NewString,ess.StringText,ess.StringLength);

  for (I=0;I<RightMargin-1;I++)
  {
    if (NewString[I]!=L' ' && NewString[I+1]==L' ')
    {
      int MoveSize=AddSize;
      if (Reminder)
      {
        MoveSize++;
        Reminder--;
      }
      if (MoveSize==0)
        break;
      wmemmove(NewString+I+1+MoveSize,NewString+I+1,RightMargin-(I+1+MoveSize));
      while (MoveSize--)
        NewString[I+1+MoveSize]=L' ';
    }
  }

  ess.StringText=NewString;
  ess.StringLength=RightMargin;
  Info.EditorControl(-1,ECTL_SETSTRING,0,&ess);
  free(NewString);
  return(TRUE);
}
