//=============================================================================
//  Filename:   UIAnimatedStatic.cpp
//	Created by Roman E. Marchenko, vortex@gsc-game.kiev.ua
//	Copyright 2004. GSC Game World
//	---------------------------------------------------------------------------
//  Статик для отображения анимированной иконки
//=============================================================================

#include "stdafx.h"
#include "UIAnimatedStatic.h"

//////////////////////////////////////////////////////////////////////////

CUIAnimatedStatic::CUIAnimatedStatic()
	:	m_uFrameCount			(0),
		m_uAnimationDuration	(0),
		m_uTimeElapsed			(0),
		m_uAnimCols				(0xffffffff),
		m_bCyclic				(true),
		m_bParamsChanged		(true),
		m_uFrameWidth			(0),
		m_uFrameHeight			(0),
		m_uCurFrame				(0xffffffff),
		m_bPlaying				(false),
		m_prevTime				(0)
{
	m_pos.set(0,0);
//.	ClipperOn();
}

//////////////////////////////////////////////////////////////////////////

void CUIAnimatedStatic::Update()
{
	if (!m_bPlaying) return;

	static u32 oneFrameDuration = 0;

	// Пересчитаем пааметры анимации
	if (m_bParamsChanged && 0 != m_uFrameCount)
	{
		// Пересчитаем время одного кадра
		oneFrameDuration = iCeil(m_uAnimationDuration / static_cast<float>(m_uFrameCount));

		SetFrame(0);

		m_bParamsChanged = false;
	}

	// Прибавляем время кадра
	m_uTimeElapsed += Device.dwTimeContinual - m_prevTime;
	m_prevTime = Device.dwTimeContinual;

	// Если анимация закончилась
	if (m_uTimeElapsed > m_uAnimationDuration)
	{
		Rewind(0);
		if (!m_bCyclic)
			Stop();
	}

	// Теперь вычисляем кадры в зависимости от времени
	u32 curFrame = m_uTimeElapsed / oneFrameDuration;

	if (curFrame != m_uCurFrame)
	{
		m_uCurFrame = curFrame;
		SetFrame(m_uCurFrame);
	}
}

//////////////////////////////////////////////////////////////////////////

void CUIAnimatedStatic::SetFrame(const u32 frameNum)
{
	//static u32 currRow = 0xffffffff, currCol = 0xffffffff;
	int currRow = frameNum / m_uAnimCols;
	int currCol = frameNum % m_uAnimCols;
	Frect texture_rect;

	texture_rect.lt.set					(m_pos.x + currCol*m_uFrameWidth, m_pos.y + currRow*m_uFrameHeight);
	texture_rect.rb.set					(m_uFrameWidth, m_uFrameHeight);
	texture_rect.rb.add					(texture_rect.lt);
	GetUIStaticItem().SetTextureRect	(texture_rect);
}

void CUIAnimatedStatic::SetAnimPos(float pos){
	R_ASSERT(pos >= 0 && pos <= 1);

	u32 curFrame = u32(m_uFrameCount*pos);

	if (curFrame != m_uCurFrame)
	{
		m_uCurFrame = curFrame;
		SetFrame(m_uCurFrame);
	}
}
//-----------------------------------------------------------------------------------------
//Static for sleep control-----------------------------------------------------------------
//-----------------------------------------------------------------------------------------
#include "../Actor_Flags.h"
#include "../Level.h"
#include "../date_time.h"
#include "UITextureMaster.h"
CUISleepStatic::CUISleepStatic():m_cur_time(0) 
{
};

void CUISleepStatic::Draw()
{
//	inherited::Draw();
	m_UIStaticItem.Render();
	m_UIStaticItem2.Render();
}

void CUISleepStatic::Update()
{
	u32 year = 0, month = 0, day = 0, hours = 0, mins = 0, secs = 0, milisecs = 0;
	split_time(Level().GetGameTime(), year, month, day, hours, mins, secs, milisecs);

	u32 start_pixel = 0, end_pixel = 0, start_pixel2 = 0, end_pixel2 = 0;
	hours += psActorSleepTime-1;
	if(hours>=24)
		hours -= 24;

	start_pixel = hours*85;
	end_pixel = (hours+7)*85;
	if(end_pixel>2048)
	{
		end_pixel2 = end_pixel - 2048;
		end_pixel = 2048;
	}
	
	Fvector2 parent_pos = GetParent()->GetWndPos();
	Fvector2 pos = GetWndPos();
	pos.x += parent_pos.x;
	pos.y += parent_pos.y;

	Frect r = Frect().set((float)start_pixel, 0.0f, (float)end_pixel, 128.0f);
	m_UIStaticItem.SetTextureRect(r);
	m_UIStaticItem.SetSize(Fvector2().set(iFloor((end_pixel-start_pixel)*UI().get_current_kx()), 128));
	m_UIStaticItem.SetPos(pos.x, pos.y);
	if(end_pixel2>0)
	{
		r.set((float)start_pixel2, 0.0f, (float)end_pixel2, 128.0f);
		m_UIStaticItem2.SetTextureRect(r);
		m_UIStaticItem2.SetSize(Fvector2().set(iFloor(end_pixel2*UI().get_current_kx()), 128));
		m_UIStaticItem2.SetPos(m_UIStaticItem.GetPosX()+m_UIStaticItem.GetSize().x, m_UIStaticItem.GetPosY());
	}
	else
		m_UIStaticItem2.SetSize(Fvector2().set(1, 1));

}

void CUISleepStatic::InitTextureEx(LPCSTR tex_name, LPCSTR sh_name)
{
	inherited::InitTextureEx(tex_name, sh_name);

	LPCSTR res_shname = UIRender->UpdateShaderName(tex_name, sh_name);
	CUITextureMaster::InitTexture(tex_name, &m_UIStaticItem2, res_shname);

	Fvector2 p = GetWndPos();
	m_UIStaticItem2.SetPos(p.x, p.y);
	p.set(1,1);
	m_UIStaticItem2.SetSize(p);
}
