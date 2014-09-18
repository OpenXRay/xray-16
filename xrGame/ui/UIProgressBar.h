#pragma once

#include "uiwindow.h"
#include "uibutton.h"
#include "../../xrServerEntities/script_export_space.h"

class CUIProgressBar : public CUIWindow
{
	friend class		CUIXmlInit;
	typedef CUIWindow	inherited;
protected:
//	bool				m_bIsHorizontal;
	enum EOrientMode
	{
		om_horz = 0,
		om_vert = 1,
		om_back = 2,
		om_down = 3,
		om_count
	}	m_orient_mode;

	Fvector2			m_ProgressPos; //x-current y-dest
	float				m_MinPos;
	float				m_MaxPos;

	float				m_CurrentLength;
	
	bool				m_bBackgroundPresent;
	Fvector2			m_BackgroundOffset;
	u32					m_last_render_frame;
	void				UpdateProgressBar();
	
public:
	bool				m_bUseColor;
	Fcolor				m_minColor;
	Fcolor				m_middleColor;
	Fcolor				m_maxColor;
	float				m_inertion;	//
public:
	CUIStatic			m_UIProgressItem;
	CUIStatic			m_UIBackgroundItem;


						CUIProgressBar				();
	virtual				~CUIProgressBar				();


			void		InitProgressBar				(Fvector2 pos, Fvector2 size, EOrientMode mode);

	void				SetRange					(float _Min, float _Max)	{ m_MinPos = _Min;  m_MaxPos = _Max; UpdateProgressBar();}
	float				GetRange_min				() 							{ return  m_MinPos; }
	float				GetRange_max				() 							{ return  m_MaxPos; }

	void				SetProgressPos				(float _Pos);
	float				GetProgressPos				()							{ return m_ProgressPos.y; }

	void				ShowBackground				(bool status)				{ m_bBackgroundPresent = status; }
	bool				IsShownBackground			()							{ return m_bBackgroundPresent; }

	virtual void		Draw						();
	virtual void		Update						();

	DECLARE_SCRIPT_REGISTER_FUNCTION
};