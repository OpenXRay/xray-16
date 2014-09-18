#pragma once

#include "UIDialogWnd.h"

class CUIStatic;
class CUITextWnd;
class CUI3tButton;
class CUIKickPlayer;
class CUIChangeMap;
class CUIXml;

class CUIChangeWeather : public CUIDialogWnd 
{
public:
					CUIChangeWeather	();
			void	InitChangeWeather	(CUIXml& xml_doc);

	virtual bool	OnKeyboardAction			(int dik, EUIMessages keyboard_action);
	virtual void	SendMessage			(CUIWindow* pWnd, s16 msg, void* pData = 0);

	virtual void	OnBtn				(int i);
	void			OnBtnCancel			();

protected:
	void			ParseWeather		();
	void			AddWeather			(const shared_str& name, const shared_str& time);
	u32				weather_counter;

	struct SWeatherData{
		CUITextWnd*	m_text;
		shared_str	m_weather_name;
		shared_str	m_weather_time;
	};

	CUITextWnd*		header;
	CUI3tButton*	btn[4];
	SWeatherData	m_data[4];
	CUIStatic*		bkgrnd;
	CUI3tButton*	btn_cancel;
};

class CUIChangeGameType :public CUIChangeWeather
{
public:
	void			InitChangeGameType	(CUIXml& xml_doc);
	virtual void	OnBtn				(int i);
};