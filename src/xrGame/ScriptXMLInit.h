#pragma once

#include "xrUICore/XML//xrUIXmlParser.h"

class CUIWindow;
class CUIFrameWindow;
class CUIStatic;
class CUITextWnd;
class CUICheckButton;
class CUISpinNum;
class CUISpinText;
class CUISpinFlt;
class CUIComboBox;
class CUIButton;
class CUI3tButton;
class CUICheckButton;
class CUITabControl;
class CUIFrameLineWnd;
class CUIEditBox;
class CUIMultiTextStatic;
class CUIAnimatedStatic;
class CUISleepStatic;
class CServerList;
class CUIMapList;
class CUIVersionList;
class CUITrackBar;
class CUIMapInfo;
class CUIMMShniaga;
class CUIScrollView;
class CUIListWnd;
class CUIListBox;
class CUIProgressBar;

class CScriptXmlInit
{
public:
    void ParseFile(const char* xml_file);
    void ParseShTexInfo(pcstr xml_file);
    void InitWindow(const char* path, int index, CUIWindow* pWnd);
    CUIFrameWindow* InitFrame(const char* path, CUIWindow* parent);
    CUIFrameLineWnd* InitFrameLine(const char* path, CUIWindow* parent);
    CUIEditBox* InitEditBox(const char* path, CUIWindow* parent);
    CUIStatic* InitStatic(const char* path, CUIWindow* parent);
    CUIStatic* InitAnimStatic(const char* path, CUIWindow* parent);
    CUIStatic* InitSleepStatic(const char* path, CUIWindow* parent);
    CUITextWnd* InitTextWnd(const char* path, CUIWindow* parent);
    CUICheckButton* InitCheck(const char* path, CUIWindow* parent);
    CUISpinNum* InitSpinNum(const char* path, CUIWindow* parent);
    CUISpinFlt* InitSpinFlt(const char* path, CUIWindow* parent);
    CUISpinText* InitSpinText(const char* path, CUIWindow* parent);
    CUIComboBox* InitComboBox(const char* path, CUIWindow* parent);
    CUI3tButton* Init3tButton(const char* path, CUIWindow* parent);

    CUITabControl* InitTab(const char* path, CUIWindow* parent);
    CServerList* InitServerList(const char* path, CUIWindow* parent);
    CUIMapList* InitMapList(const char* path, CUIWindow* parent);
    CUIVersionList* InitVerList(const char* path, CUIWindow* parent);
    CUIMapInfo* InitMapInfo(const char* path, CUIWindow* parent);
    CUITrackBar* InitTrackBar(const char* path, CUIWindow* parent);
    CUIEditBox* InitCDkey(const char* path, CUIWindow* parent);
    CUIEditBox* InitMPPlayerName(const char* path, CUIWindow* parent);
    CUIMMShniaga* InitMMShniaga(const char* path, CUIWindow* parent);
    CUIWindow* InitKeyBinding(const char* path, CUIWindow* parent);
    CUIScrollView* InitScrollView(const char* path, CUIWindow* parent);
    CUIListWnd* InitListWnd(pcstr path, CUIWindow* parent);
    CUIListBox* InitListBox(const char* path, CUIWindow* parent);
    CUIProgressBar* InitProgressBar(const char* path, CUIWindow* parent);

protected:
    CUIXml m_xml;
};
