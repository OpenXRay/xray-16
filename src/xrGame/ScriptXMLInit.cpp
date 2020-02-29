#include "pch_script.h"
#include "ScriptXMLInit.h"
#include "ui/UIXmlInit.h"
#include "xrUICore/XML/UITextureMaster.h"
#include "xrUICore/Buttons/UICheckButton.h"
#include "xrUICore/SpinBox/UISpinNum.h"
#include "xrUICore/SpinBox/UISpinText.h"
#include "xrUICore/ComboBox/UIComboBox.h"
#include "xrUICore/TabControl/UITabControl.h"
#include "xrUICore/Windows/UIFrameWindow.h"
#include "ui/UILabel.h"
#include "ui/ServerList.h"
#include "ui/UIMapList.h"
#include "ui/UIVersionList.h"
#include "ui/UIKeyBinding.h"
#include "xrUICore/EditBox/UIEditBox.h"
#include "xrUICore/Static/UIAnimatedStatic.h"
#include "ui/UISleepStatic.h"
#include "xrUICore/TrackBar/UITrackBar.h"
#include "ui/UICDkey.h"
#include "ui/UIMapInfo.h"
#include "ui/UIMMShniaga.h"
#include "xrUICore/ScrollView/UIScrollView.h"
#include "xrUICore/ListWnd/UIListWnd.h"
#include "xrUICore/ProgressBar/UIProgressBar.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

void _attach_child(CUIWindow* _child, CUIWindow* _parent)
{
    if (!_parent)
        return;

    _child->SetAutoDelete(true);
    CUIScrollView* _parent_scroll = smart_cast<CUIScrollView*>(_parent);
    if (_parent_scroll)
        _parent_scroll->AddWindow(_child, true);
    else
        _parent->AttachChild(_child);
}

void CScriptXmlInit::ParseFile(const char* xml_file) { m_xml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, xml_file); }

void CScriptXmlInit::ParseShTexInfo(pcstr xml_file)
{
    CUITextureMaster::ParseShTexInfo(xml_file);
}

void CScriptXmlInit::InitWindow(const char* path, int index, CUIWindow* pWnd)
{
    CUIXmlInit::InitWindow(m_xml, path, index, pWnd);
}

CUIFrameWindow* CScriptXmlInit::InitFrame(const char* path, CUIWindow* parent)
{
    CUIFrameWindow* pWnd = new CUIFrameWindow();
    CUIXmlInit::InitFrameWindow(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUIFrameLineWnd* CScriptXmlInit::InitFrameLine(const char* path, CUIWindow* parent)
{
    CUIFrameLineWnd* pWnd = new CUIFrameLineWnd();
    CUIXmlInit::InitFrameLine(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUIEditBox* CScriptXmlInit::InitEditBox(const char* path, CUIWindow* parent)
{
    CUIEditBox* pWnd = new CUIEditBox();
    CUIXmlInit::InitEditBox(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUIStatic* CScriptXmlInit::InitStatic(const char* path, CUIWindow* parent)
{
    CUIStatic* pWnd = new CUIStatic();
    CUIXmlInit::InitStatic(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUITextWnd* CScriptXmlInit::InitTextWnd(const char* path, CUIWindow* parent)
{
    CUITextWnd* pWnd = new CUITextWnd();
    CUIXmlInit::InitTextWnd(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUIStatic* CScriptXmlInit::InitAnimStatic(const char* path, CUIWindow* parent)
{
    CUIAnimatedStatic* pWnd = new CUIAnimatedStatic();
    CUIXmlInit::InitAnimatedStatic(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUIStatic* CScriptXmlInit::InitSleepStatic(const char* path, CUIWindow* parent)
{
    CUISleepStatic* pWnd = new CUISleepStatic();
    CUIXmlInit::InitSleepStatic(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUIScrollView* CScriptXmlInit::InitScrollView(const char* path, CUIWindow* parent)
{
    CUIScrollView* pWnd = new CUIScrollView();
    CUIXmlInit::InitScrollView(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUIListWnd* CScriptXmlInit::InitListWnd(pcstr path, CUIWindow* parent)
{
    CUIListWnd* pWnd = new CUIListWnd();
    CUIXmlInit::InitListWnd(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUIListBox* CScriptXmlInit::InitListBox(const char* path, CUIWindow* parent)
{
    CUIListBox* pWnd = new CUIListBox();
    CUIXmlInit::InitListBox(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUICheckButton* CScriptXmlInit::InitCheck(const char* path, CUIWindow* parent)
{
    CUICheckButton* pWnd = new CUICheckButton();
    CUIXmlInit::InitCheck(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUISpinNum* CScriptXmlInit::InitSpinNum(const char* path, CUIWindow* parent)
{
    CUISpinNum* pWnd = new CUISpinNum();
    CUIXmlInit::InitSpin(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUISpinFlt* CScriptXmlInit::InitSpinFlt(const char* path, CUIWindow* parent)
{
    CUISpinFlt* pWnd = new CUISpinFlt();
    CUIXmlInit::InitSpin(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUISpinText* CScriptXmlInit::InitSpinText(const char* path, CUIWindow* parent)
{
    CUISpinText* pWnd = new CUISpinText();
    CUIXmlInit::InitSpin(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUIComboBox* CScriptXmlInit::InitComboBox(const char* path, CUIWindow* parent)
{
    CUIComboBox* pWnd = new CUIComboBox();
    CUIXmlInit::InitComboBox(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUI3tButton* CScriptXmlInit::Init3tButton(const char* path, CUIWindow* parent)
{
    CUI3tButton* pWnd = new CUI3tButton();
    CUIXmlInit::Init3tButton(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUITabControl* CScriptXmlInit::InitTab(const char* path, CUIWindow* parent)
{
    CUITabControl* pWnd = new CUITabControl();
    CUIXmlInit::InitTabControl(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CServerList* CScriptXmlInit::InitServerList(const char* path, CUIWindow* parent)
{
    CServerList* pWnd = new CServerList();
    pWnd->InitFromXml(m_xml, path);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUIMapList* CScriptXmlInit::InitMapList(const char* path, CUIWindow* parent)
{
    CUIMapList* pWnd = new CUIMapList();
    pWnd->InitFromXml(m_xml, path);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUIVersionList* CScriptXmlInit::InitVerList(const char* path, CUIWindow* parent)
{
    CUIVersionList* pWnd = new CUIVersionList();
    pWnd->InitFromXml(m_xml, path);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUIMMShniaga* CScriptXmlInit::InitMMShniaga(const char* path, CUIWindow* parent)
{
    CUIMMShniaga* pWnd = new CUIMMShniaga();
    pWnd->InitShniaga(m_xml, path);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUIMapInfo* CScriptXmlInit::InitMapInfo(const char* path, CUIWindow* parent)
{
    CUIMapInfo* pWnd = new CUIMapInfo();
    CUIXmlInit::InitWindow(m_xml, path, 0, pWnd);
    pWnd->InitMapInfo(pWnd->GetWndPos(), pWnd->GetWndSize());
    _attach_child(pWnd, parent);
    return pWnd;
}

CUIWindow* CScriptXmlInit::InitKeyBinding(const char* path, CUIWindow* parent)
{
    CUIKeyBinding* pWnd = new CUIKeyBinding();
    pWnd->InitFromXml(m_xml, path);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUITrackBar* CScriptXmlInit::InitTrackBar(const char* path, CUIWindow* parent)
{
    CUITrackBar* pWnd = new CUITrackBar();
    CUIXmlInit::InitTrackBar(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUIProgressBar* CScriptXmlInit::InitProgressBar(const char* path, CUIWindow* parent)
{
    CUIProgressBar* pWnd = new CUIProgressBar();
    CUIXmlInit::InitProgressBar(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUIEditBox* CScriptXmlInit::InitCDkey(const char* path, CUIWindow* parent)
{
    CUICDkey* pWnd = new CUICDkey();
    CUIXmlInit::InitEditBox(m_xml, path, 0, pWnd);
    pWnd->assign_callbacks();
    _attach_child(pWnd, parent);
    pWnd->SetCurrentOptValue();
    return pWnd;
}

CUIEditBox* CScriptXmlInit::InitMPPlayerName(const char* path, CUIWindow* parent)
{
    CUIMPPlayerName* pWnd = new CUIMPPlayerName();
    CUIXmlInit::InitEditBox(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

SCRIPT_EXPORT(CScriptXmlInit, (),
{
    module(luaState)
    [
        class_<CScriptXmlInit>("CScriptXmlInit")
            .def(constructor<>())
            .def("ParseFile", &CScriptXmlInit::ParseFile)
            .def("ParseShTexInfo", &CScriptXmlInit::ParseShTexInfo)
            .def("InitWindow", &CScriptXmlInit::InitWindow)
            .def("InitFrame", &CScriptXmlInit::InitFrame)
            .def("InitFrameLine", &CScriptXmlInit::InitFrameLine)
            .def("InitEditBox", &CScriptXmlInit::InitEditBox)
            .def("InitStatic", &CScriptXmlInit::InitStatic)
            .def("InitTextWnd", &CScriptXmlInit::InitTextWnd)
            .def("InitLabel", &CScriptXmlInit::InitStatic)
            .def("InitAnimStatic", &CScriptXmlInit::InitAnimStatic)
            .def("InitSleepStatic", &CScriptXmlInit::InitSleepStatic)
            .def("Init3tButton", &CScriptXmlInit::Init3tButton)
            .def("InitCheck", &CScriptXmlInit::InitCheck)
            .def("InitSpinNum", &CScriptXmlInit::InitSpinNum)
            .def("InitSpinFlt", &CScriptXmlInit::InitSpinFlt)
            .def("InitSpinText", &CScriptXmlInit::InitSpinText)
            .def("InitComboBox", &CScriptXmlInit::InitComboBox)
            .def("InitTab", &CScriptXmlInit::InitTab)
            .def("InitServerList", &CScriptXmlInit::InitServerList)
            .def("InitMapList", &CScriptXmlInit::InitMapList)
            .def("InitVerList", &CScriptXmlInit::InitVerList)
            .def("InitMapInfo", &CScriptXmlInit::InitMapInfo)
            .def("InitTrackBar", &CScriptXmlInit::InitTrackBar)
            .def("InitCDkey", &CScriptXmlInit::InitCDkey)
            .def("InitMPPlayerName", &CScriptXmlInit::InitMPPlayerName)
            .def("InitKeyBinding", &CScriptXmlInit::InitKeyBinding)
            .def("InitMMShniaga", &CScriptXmlInit::InitMMShniaga)
            .def("InitScrollView", &CScriptXmlInit::InitScrollView)
            .def("InitList", &CScriptXmlInit::InitListWnd)
            .def("InitListBox", &CScriptXmlInit::InitListBox)
            .def("InitProgressBar", &CScriptXmlInit::InitProgressBar)
    ];
});
