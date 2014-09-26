#include "stdafx.h"
#include "UIChangeWeather.h"
#include "UIXmlInit.h"
#include "UI3tButton.h"
#include "../game_cl_teamdeathmatch.h"
#include "UIKickPlayer.h"
#include "UIChangeMap.h"
//#include "UIMapList.h"

CUIChangeWeather::CUIChangeWeather(){
	bkgrnd = xr_new<CUIStatic>(); 
	bkgrnd->SetAutoDelete(true);
	AttachChild(bkgrnd);

	header = xr_new<CUITextWnd>();
	header->SetAutoDelete(true);
	AttachChild(header);

	btn_cancel = xr_new<CUI3tButton>();
	btn_cancel->SetAutoDelete(true);
	AttachChild(btn_cancel);

	for (int i = 0; i<4; i++)
	{
		btn[i] = xr_new<CUI3tButton>();
		btn[i]->SetAutoDelete(true);
		AttachChild(btn[i]);

		m_data[i].m_text = xr_new<CUITextWnd>();
		m_data[i].m_text->SetAutoDelete(true);
		AttachChild(m_data[i].m_text);
	}

	weather_counter = 0;
}

void CUIChangeWeather::InitChangeWeather(CUIXml& xml_doc)
{
	CUIXmlInit::InitWindow(xml_doc, "change_weather", 0, this);

	CUIXmlInit::InitTextWnd(xml_doc, "change_weather:header", 0, header);
	CUIXmlInit::InitStatic(xml_doc, "change_weather:background", 0, bkgrnd);

	string256 _path;
	for (int i = 0; i<4; i++){
		xr_sprintf(_path, "change_weather:btn_%d", i + 1);
		CUIXmlInit::Init3tButton(xml_doc, _path, 0, btn[i]);
		xr_sprintf(_path, "change_weather:txt_%d", i + 1);
		CUIXmlInit::InitTextWnd(xml_doc, _path, 0, m_data[i].m_text);
	}

	CUIXmlInit::Init3tButton(xml_doc, "change_weather:btn_cancel", 0, btn_cancel);

	ParseWeather();
}

void CUIChangeWeather::SendMessage(CUIWindow* pWnd, s16 msg, void* pData){
	if (BUTTON_CLICKED == msg)
	{
		if (btn_cancel == pWnd)
			OnBtnCancel();
		for (int i=0; i<4; i++){
			if (btn[i] == pWnd){
				OnBtn(i);
				return;
			}
		}
	}

}

#include <dinput.h>

bool CUIChangeWeather::OnKeyboardAction(int dik, EUIMessages keyboard_action){
	CUIDialogWnd::OnKeyboardAction(dik, keyboard_action);
	if (WINDOW_KEY_PRESSED == keyboard_action){
		if (DIK_ESCAPE == dik){
			OnBtnCancel();
			return true;
		}
		if (dik >= DIK_1 && dik <= DIK_4){
			OnBtn(dik - DIK_1);
			return true;
		}
	}

	return false;
}

#include "../../xrEngine/xr_ioconsole.h"

void CUIChangeWeather::OnBtn(int i)
{
	string1024				command;
	xr_sprintf				(command, "cl_votestart changeweather %s %s", *m_data[i].m_weather_name, *m_data[i].m_weather_time);
	Console->Execute		(command);
	HideDialog							();
}

void CUIChangeWeather::OnBtnCancel()
{
	HideDialog							();
}

#include "UIMapList.h"
#include "../UIGameCustom.h"

void CUIChangeWeather::ParseWeather()
{
	weather_counter = 0;

	GAME_WEATHERS game_weathers = gMapListHelper.GetGameWeathers();
	GAME_WEATHERS_CIT it		= game_weathers.begin();
	GAME_WEATHERS_CIT it_e		= game_weathers.end();
	
	for( ;it!=it_e; ++it)
	{
		AddWeather			( (*it).m_weather_name, (*it).m_start_time);
	}
};

void CUIChangeWeather::AddWeather(const shared_str& weather, const shared_str& time){
//	m_data[weather_counter].m_text->SetTextST	(weather.c_str());
	m_data[weather_counter].m_weather_name		= weather;
	m_data[weather_counter].m_weather_time		= time;
	weather_counter++;
}

void CUIChangeGameType::InitChangeGameType(CUIXml& xml_doc)
{
	CUIXmlInit::InitWindow(xml_doc, "change_gametype", 0, this);

	CUIXmlInit::InitTextWnd(xml_doc, "change_gametype:header", 0, header);
	CUIXmlInit::InitStatic(xml_doc, "change_gametype:background", 0, bkgrnd);

	string256 _path;
	for (int i = 0; i<4; i++)
	{
		xr_sprintf(_path, "change_gametype:btn_%d", i + 1);
		CUIXmlInit::Init3tButton(xml_doc, _path, 0, btn[i]);
		xr_sprintf(_path, "change_gametype:txt_%d", i + 1);
		CUIXmlInit::InitTextWnd(xml_doc, _path, 0, m_data[i].m_text);
		m_data[i].m_weather_name = xml_doc.ReadAttrib(_path,0,"id");
	}

	CUIXmlInit::Init3tButton(xml_doc, "change_gametype:btn_cancel", 0, btn_cancel);
}

void CUIChangeGameType::OnBtn(int i)
{
	string1024				command;
	xr_sprintf				(command, "cl_votestart changegametype %s", m_data[i].m_weather_name.c_str());
	Console->Execute		(command);
	HideDialog				();
}
