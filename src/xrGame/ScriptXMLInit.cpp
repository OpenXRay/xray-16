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
#include "xrUICore/Hint/UIHint.h"
#include "ui/UILabel.h"
#include "ui/ServerList.h"
#include "ui/UIMapList.h"
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

void CScriptXmlInit::ParseFile(LPCSTR xml_file) { m_xml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, xml_file); }

void CScriptXmlInit::ParseShTexInfo(pcstr xml_file)
{
    CUITextureMaster::ParseShTexInfo(xml_file);
}

void CScriptXmlInit::InitWindow(LPCSTR path, int index, CUIWindow* pWnd)
{
    CUIXmlInit::InitWindow(m_xml, path, index, pWnd);
}

UIHint* CScriptXmlInit::InitHint(pcstr path, CUIWindow* parent)
{
    UIHint* pWnd = xr_new<UIHint>();
    pWnd->init_from_xml(m_xml, path);
    _attach_child(pWnd, parent);
	return pWnd;
}

CUIFrameWindow* CScriptXmlInit::InitFrame(LPCSTR path, CUIWindow* parent)
{
    CUIFrameWindow* pWnd = xr_new<CUIFrameWindow>(path);
    CUIXmlInit::InitFrameWindow(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUIFrameLineWnd* CScriptXmlInit::InitFrameLine(LPCSTR path, CUIWindow* parent)
{
    CUIFrameLineWnd* pWnd = xr_new<CUIFrameLineWnd>(path);
    CUIXmlInit::InitFrameLine(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUIEditBox* CScriptXmlInit::InitEditBox(LPCSTR path, CUIWindow* parent)
{
    CUIEditBox* pWnd = xr_new<CUIEditBox>();
    CUIXmlInit::InitEditBox(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUIStatic* CScriptXmlInit::InitStatic(LPCSTR path, CUIWindow* parent)
{
    CUIStatic* pWnd = xr_new<CUIStatic>(path);
    CUIXmlInit::InitStatic(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUIStatic* CScriptXmlInit::InitTextWnd(LPCSTR path, CUIWindow* parent)
{
    auto* pWnd = xr_new<CUIStatic>(path);
    CUIXmlInit::InitStatic(m_xml, path, 0, pWnd, true, true);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUIStatic* CScriptXmlInit::InitAnimStatic(LPCSTR path, CUIWindow* parent)
{
    CUIAnimatedStatic* pWnd = xr_new<CUIAnimatedStatic>();
    CUIXmlInit::InitAnimatedStatic(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUIStatic* CScriptXmlInit::InitSleepStatic(LPCSTR path, CUIWindow* parent)
{
    CUISleepStatic* pWnd = xr_new<CUISleepStatic>();
    CUIXmlInit::InitSleepStatic(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUIScrollView* CScriptXmlInit::InitScrollView(LPCSTR path, CUIWindow* parent)
{
    CUIScrollView* pWnd = xr_new<CUIScrollView>();
    CUIXmlInit::InitScrollView(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUIListWnd* CScriptXmlInit::InitListWnd(pcstr path, CUIWindow* parent)
{
    CUIListWnd* pWnd = xr_new<CUIListWnd>();
    CUIXmlInit::InitListWnd(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUIListBox* CScriptXmlInit::InitListBox(LPCSTR path, CUIWindow* parent)
{
    CUIListBox* pWnd = xr_new<CUIListBox>();
    CUIXmlInit::InitListBox(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUICheckButton* CScriptXmlInit::InitCheck(LPCSTR path, CUIWindow* parent)
{
    CUICheckButton* pWnd = xr_new<CUICheckButton>();
    CUIXmlInit::InitCheck(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUISpinNum* CScriptXmlInit::InitSpinNum(LPCSTR path, CUIWindow* parent)
{
    CUISpinNum* pWnd = xr_new<CUISpinNum>();
    CUIXmlInit::InitSpin(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUISpinFlt* CScriptXmlInit::InitSpinFlt(LPCSTR path, CUIWindow* parent)
{
    CUISpinFlt* pWnd = xr_new<CUISpinFlt>();
    CUIXmlInit::InitSpin(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUISpinText* CScriptXmlInit::InitSpinText(LPCSTR path, CUIWindow* parent)
{
    CUISpinText* pWnd = xr_new<CUISpinText>();
    CUIXmlInit::InitSpin(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUIComboBox* CScriptXmlInit::InitComboBox(LPCSTR path, CUIWindow* parent)
{
    CUIComboBox* pWnd = xr_new<CUIComboBox>();
    CUIXmlInit::InitComboBox(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUI3tButton* CScriptXmlInit::Init3tButton(LPCSTR path, CUIWindow* parent)
{
    CUI3tButton* pWnd = xr_new<CUI3tButton>();
    CUIXmlInit::Init3tButton(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUITabControl* CScriptXmlInit::InitTab(LPCSTR path, CUIWindow* parent)
{
    CUITabControl* pWnd = xr_new<CUITabControl>();
    CUIXmlInit::InitTabControl(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CServerList* CScriptXmlInit::InitServerList(LPCSTR path, CUIWindow* parent)
{
    CServerList* pWnd = xr_new<CServerList>();
    pWnd->InitFromXml(m_xml, path);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUIMapList* CScriptXmlInit::InitMapList(LPCSTR path, CUIWindow* parent)
{
    CUIMapList* pWnd = xr_new<CUIMapList>();
    pWnd->InitFromXml(m_xml, path);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUIMMShniaga* CScriptXmlInit::InitMMShniaga(LPCSTR path, CUIWindow* parent)
{
    CUIMMShniaga* pWnd = xr_new<CUIMMShniaga>();
    pWnd->InitShniaga(m_xml, path);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUIMapInfo* CScriptXmlInit::InitMapInfo(LPCSTR path, CUIWindow* parent)
{
    CUIMapInfo* pWnd = xr_new<CUIMapInfo>();
    CUIXmlInit::InitWindow(m_xml, path, 0, pWnd);
    pWnd->InitMapInfo(pWnd->GetWndPos(), pWnd->GetWndSize());
    _attach_child(pWnd, parent);
    return pWnd;
}

CUIWindow* CScriptXmlInit::InitKeyBinding(LPCSTR path, CUIWindow* parent)
{
    CUIKeyBinding* pWnd = xr_new<CUIKeyBinding>();
    pWnd->InitFromXml(m_xml, path);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUITrackBar* CScriptXmlInit::InitTrackBar(LPCSTR path, CUIWindow* parent)
{
    CUITrackBar* pWnd = xr_new<CUITrackBar>();
    CUIXmlInit::InitTrackBar(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUIProgressBar* CScriptXmlInit::InitProgressBar(LPCSTR path, CUIWindow* parent)
{
    CUIProgressBar* pWnd = xr_new<CUIProgressBar>();
    CUIXmlInit::InitProgressBar(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

CUIEditBox* CScriptXmlInit::InitCDkey(LPCSTR path, CUIWindow* parent)
{
    CUICDkey* pWnd = xr_new<CUICDkey>();
    CUIXmlInit::InitEditBox(m_xml, path, 0, pWnd);
    pWnd->assign_callbacks();
    _attach_child(pWnd, parent);
    pWnd->SetCurrentOptValue();
    return pWnd;
}

CUIEditBox* CScriptXmlInit::InitMPPlayerName(LPCSTR path, CUIWindow* parent)
{
    CUIMPPlayerName* pWnd = xr_new<CUIMPPlayerName>();
    CUIXmlInit::InitEditBox(m_xml, path, 0, pWnd);
    _attach_child(pWnd, parent);
    return pWnd;
}

SCRIPT_EXPORT(CScriptXmlInit, (),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CScriptXmlInit>("CScriptXmlInit")
            .def(constructor<>())
            .def("ParseFile", &CScriptXmlInit::ParseFile)
            .def("ParseShTexInfo", &CScriptXmlInit::ParseShTexInfo)
            .def("InitWindow", &CScriptXmlInit::InitWindow)
            .def("InitHint", &CScriptXmlInit::InitHint)
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
