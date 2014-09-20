#include "StdAfx.h"
#include "UIPdaKillMessage.h"
#include "UIInventoryUtilities.h"
#include "../Include/xrRender/UIShader.h"

const int INDENT = 3;

CUIPdaKillMessage::CUIPdaKillMessage()
{
	AttachChild						(&m_victim_name);
	m_victim_name.SetTextComplexMode(false);
	AttachChild						(&m_killer_name);
	m_killer_name.SetTextComplexMode(false);	
	AttachChild						(&m_initiator);
	AttachChild						(&m_ext_info);
}

void CUIPdaKillMessage::Init(KillMessageStruct& msg, CGameFont* F)
{
	float x		= 0;
	float width	= 0;
	m_killer_name.SetFont			(F);
	m_victim_name.SetFont			(F);
	width = InitText(m_killer_name, x, msg.m_killer);		x += width ? width + INDENT : 0;
	width = InitIcon(m_initiator,   x, msg.m_initiator);	x += width ? width + INDENT : 0;
	width = InitText(m_victim_name, x, msg.m_victim);		x += width ? width + INDENT : 0;
			InitIcon(m_ext_info,	x, msg.m_ext_info);

	Fvector2 sz		= GetWndSize();
	sz.x			= _max(sz.x, x+m_ext_info.GetWidth());
	SetWndSize		(sz);
	
	SetColorAnimation			("ui_main_msgs_short", LA_ONLYALPHA|LA_TEXTCOLOR|LA_TEXTURECOLOR, 5000.0f);
}

float CUIPdaKillMessage::InitText(CUITextWnd& refStatic, float x, PlayerInfo& info)
{

	if ( 0 == xr_strlen(info.m_name))
		return 0.0f;

	CGameFont* pFont					= refStatic.GetFont();
	float _eps							= pFont->SizeOf_(' ');
	UI().ClientToScreenScaledWidth		(_eps); //add one letter

	float height						= pFont->CurrentHeight_();
	float y								= (GetHeight() - height)/2;

	refStatic.SetWndPos					(Fvector2().set(x, y));
	refStatic.SetHeight					(GetHeight());
	refStatic.SetEllipsis				(true);
	refStatic.SetText					(info.m_name.c_str());
	refStatic.AdjustWidthToText			();
	refStatic.SetWidth					(refStatic.GetWidth()+_eps);
	refStatic.SetTextColor				(info.m_color);

	return		refStatic.GetWidth		();
}

float CUIPdaKillMessage::InitIcon(CUIStatic& refStatic, float x, IconInfo& info)
{
	if ( 0 == info.m_rect.width())
		return 0;

	if (!info.m_shader->inited())
		return 0;

	float y								= 0;
	float selfHeight					= GetHeight();
	float scale							= 0;
	Frect rect							= info.m_rect;

	float width							= rect.width();
	float height						= rect.height();
	
	scale = selfHeight/height;
	if (scale > 1)
		scale = 1;
	width  								= width*scale;
	height 								= height*scale;
	y									= (selfHeight - height) /2;

	refStatic.SetWndPos					(Fvector2().set(x,y));
	refStatic.SetWndSize				(Fvector2().set(width,height));

	refStatic.SetTextureRect			(info.m_rect);
	refStatic.SetShader					(info.m_shader);
	refStatic.SetStretchTexture			(true);

	return width;
}