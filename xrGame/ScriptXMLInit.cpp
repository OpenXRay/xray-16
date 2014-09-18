#include "pch_script.h"
#include "ScriptXmlInit.h"
#include "ui\UIXmlInit.h"
#include "ui\UITextureMaster.h"
#include "ui\UICheckButton.h"
#include "ui\UISpinNum.h"
#include "ui\UISpinText.h"
#include "ui\UIComboBox.h"
#include "ui\UITabControl.h"
#include "ui\UIFrameWindow.h"
#include "ui\UILabel.h"
#include "ui\ServerList.h"
#include "ui\UIMapList.h"
#include "ui\UIKeyBinding.h"
#include "ui\UIEditBox.h"
#include "ui\UIAnimatedStatic.h"
#include "ui\UITrackBar.h"
#include "ui\UICDkey.h"
#include "ui\UIMapInfo.h"
#include "ui\UIMMShniaga.h"
#include "ui\UIScrollView.h"
#include "ui\UIProgressBar.h"

using namespace luabind;

void _attach_child(CUIWindow* _child, CUIWindow* _parent)
{
	if(!_parent)					return;

	_child->SetAutoDelete			(true);
	CUIScrollView* _parent_scroll	= smart_cast<CUIScrollView*>(_parent);
	if(_parent_scroll)
		_parent_scroll->AddWindow	(_child, true);
	else
		_parent->AttachChild		(_child);
}

void CScriptXmlInit::ParseFile(LPCSTR xml_file)
{
	m_xml.Load(CONFIG_PATH, UI_PATH, xml_file);
}

void CScriptXmlInit::InitWindow(LPCSTR path, int index, CUIWindow* pWnd)
{
	CUIXmlInit::InitWindow(m_xml, path, index, pWnd);
}


CUIFrameWindow*	CScriptXmlInit::InitFrame(LPCSTR path, CUIWindow* parent)
{
	CUIFrameWindow* pWnd = xr_new<CUIFrameWindow>();
	CUIXmlInit::InitFrameWindow(m_xml, path, 0, pWnd);
	_attach_child(pWnd, parent);
	return pWnd;
}


CUIFrameLineWnd* CScriptXmlInit::InitFrameLine(LPCSTR path, CUIWindow* parent)
{
	CUIFrameLineWnd* pWnd = xr_new<CUIFrameLineWnd>();
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
	CUIStatic* pWnd = xr_new<CUIStatic>();
	CUIXmlInit::InitStatic(m_xml, path, 0, pWnd);
	_attach_child(pWnd, parent);
	return pWnd;
}

CUITextWnd* CScriptXmlInit::InitTextWnd(LPCSTR path, CUIWindow* parent)
{
	CUITextWnd* pWnd = xr_new<CUITextWnd>();
	CUIXmlInit::InitTextWnd(m_xml, path, 0, pWnd);
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

CUIListBox*	CScriptXmlInit::InitListBox(LPCSTR path, CUIWindow* parent)
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
	CUIMMShniaga* pWnd	= xr_new<CUIMMShniaga>();
	pWnd->InitShniaga	(m_xml, path);
	_attach_child		(pWnd, parent);
	return pWnd;
}

CUIMapInfo* CScriptXmlInit::InitMapInfo(LPCSTR path, CUIWindow* parent){
	CUIMapInfo* pWnd	= xr_new<CUIMapInfo>();
	CUIXmlInit::InitWindow(m_xml,path,0,pWnd);
	pWnd->InitMapInfo(pWnd->GetWndPos(),pWnd->GetWndSize());
	_attach_child		(pWnd, parent);
	return pWnd;	
}

CUIWindow* CScriptXmlInit::InitKeyBinding(LPCSTR path, CUIWindow* parent){
	CUIKeyBinding* pWnd				= xr_new<CUIKeyBinding>();
	pWnd->InitFromXml				(m_xml, path);	
	_attach_child					(pWnd, parent);
	return							pWnd;
}

CUITrackBar* CScriptXmlInit::InitTrackBar(LPCSTR path, CUIWindow* parent){
	CUITrackBar* pWnd				= xr_new<CUITrackBar>();
	CUIXmlInit::InitTrackBar		(m_xml, path, 0, pWnd);
	_attach_child					(pWnd, parent);
	return							pWnd;	
}

CUIProgressBar* CScriptXmlInit::InitProgressBar(LPCSTR path, CUIWindow* parent)
{
	CUIProgressBar* pWnd			= xr_new<CUIProgressBar>();
	CUIXmlInit::InitProgressBar		(m_xml, path, 0, pWnd);
	_attach_child					(pWnd, parent);
	return							pWnd;	
}

CUIEditBox* CScriptXmlInit::InitCDkey(LPCSTR path, CUIWindow* parent)
{
	CUICDkey* pWnd					= xr_new<CUICDkey>();
	CUIXmlInit::InitEditBox			(m_xml, path, 0, pWnd);
	pWnd->assign_callbacks			( );
	_attach_child					(pWnd, parent);
	pWnd->SetCurrentOptValue		();
	return							pWnd;	
}

CUIEditBox* CScriptXmlInit::InitMPPlayerName(LPCSTR path, CUIWindow* parent)
{
	CUIMPPlayerName* pWnd			= xr_new<CUIMPPlayerName>();
	CUIXmlInit::InitEditBox			(m_xml, path, 0, pWnd);
	_attach_child					(pWnd, parent);
	return							pWnd;	
}

#pragma optimize("s",on)
void CScriptXmlInit::script_register(lua_State *L){
	module(L)
	[
		class_<CScriptXmlInit>			("CScriptXmlInit")
		.def(							constructor<>())
		.def("ParseFile",				&CScriptXmlInit::ParseFile)
		.def("InitWindow",				&CScriptXmlInit::InitWindow)
		.def("InitFrame",				&CScriptXmlInit::InitFrame)
		.def("InitFrameLine",			&CScriptXmlInit::InitFrameLine)
		.def("InitEditBox",				&CScriptXmlInit::InitEditBox)		
		.def("InitStatic",				&CScriptXmlInit::InitStatic)
		.def("InitTextWnd",				&CScriptXmlInit::InitTextWnd)
		.def("InitAnimStatic",			&CScriptXmlInit::InitAnimStatic)		
		.def("InitSleepStatic",			&CScriptXmlInit::InitSleepStatic)		
		.def("Init3tButton",			&CScriptXmlInit::Init3tButton)
		.def("InitCheck",				&CScriptXmlInit::InitCheck)
		.def("InitSpinNum",				&CScriptXmlInit::InitSpinNum)
		.def("InitSpinFlt",				&CScriptXmlInit::InitSpinFlt)
		.def("InitSpinText",			&CScriptXmlInit::InitSpinText)
		.def("InitComboBox",			&CScriptXmlInit::InitComboBox)		
		.def("InitTab",					&CScriptXmlInit::InitTab)
		.def("InitServerList",			&CScriptXmlInit::InitServerList)
		.def("InitMapList",				&CScriptXmlInit::InitMapList)
		.def("InitMapInfo",				&CScriptXmlInit::InitMapInfo)
		.def("InitTrackBar",			&CScriptXmlInit::InitTrackBar)
		.def("InitCDkey",				&CScriptXmlInit::InitCDkey)
		.def("InitMPPlayerName",		&CScriptXmlInit::InitMPPlayerName)
		.def("InitKeyBinding",			&CScriptXmlInit::InitKeyBinding)
		.def("InitMMShniaga",			&CScriptXmlInit::InitMMShniaga)
		.def("InitScrollView",			&CScriptXmlInit::InitScrollView)
		.def("InitListBox",				&CScriptXmlInit::InitListBox)
		.def("InitProgressBar",			&CScriptXmlInit::InitProgressBar)
	];

}
