#include "stdafx.h"
#include "UIFrameLine.h"
#include "../hudmanager.h"
#include "UITextureMaster.h"

CUIFrameLine::CUIFrameLine()
	:	uFlags					(0),
		iSize					(0),
		bHorizontalOrientation	(true),
		bStretchTexture			(false)
{
	iPos.set(0, 0);
	m_parent_wnd_size.set(0.0f, 0.0f);
}

void CUIFrameLine::InitFrameLine(Fvector2 pos, float size, bool horizontal, DWORD align)
{
	SetPos			(pos);
	SetSize			(size);
	SetAlign		(align);
	SetOrientation	(horizontal);
}

void CUIFrameLine::InitTexture(LPCSTR texture, LPCSTR sh_name)
{
	string256		buf;

	CUITextureMaster::InitTexture(strconcat(sizeof(buf),buf,texture,"_back"),sh_name,	&elements[flBack]);
	CUITextureMaster::InitTexture(strconcat(sizeof(buf),buf,texture,"_b"),	sh_name,	&elements[flFirst]);
	CUITextureMaster::InitTexture(strconcat(sizeof(buf),buf,texture,"_e"),	sh_name,	&elements[flSecond]);
}

void CUIFrameLine::SetColor(u32 cl)
{
	for (int i = 0; i < flMax; ++i)
		elements[i].SetColor(cl);
}

void CUIFrameLine::UpdateSize()
{
	VERIFY(g_bRendering);

	float f_width		= elements[flFirst].GetOriginalRect().width();
	float f_height		= elements[flFirst].GetOriginalRect().height();
	elements[flFirst].SetPos(iPos.x, iPos.y);

	// Right or bottom texture
	float s_width		= elements[flSecond].GetOriginalRect().width();
	float s_height		= elements[flSecond].GetOriginalRect().height();
	
	if(bHorizontalOrientation && UI()->is_16_9_mode())
		s_width			/= 1.2f;

	if(bHorizontalOrientation)
		elements[flSecond].SetPos(iPos.x + iSize - s_width, iPos.y);
	else
		elements[flSecond].SetPos(iPos.x, iPos.y + iSize - s_height);

	// Dimentions of element textures must be the same
	if (bHorizontalOrientation)
		R_ASSERT(s_height == f_height);
	else
		R_ASSERT(f_width == s_width);


	// Now stretch back texture to remaining space
	float back_width, back_height;

	if (bHorizontalOrientation)
	{
		back_width	= iSize - f_width - s_width;
		back_height	= f_height;

		// Size of frameline must be equal or greater than sum of size of two side textures
		R_ASSERT(back_width > 0);
	}
	else
	{
		back_width	= f_width;
		back_height	= iSize - f_height - s_height;

		// Size of frameline must be equal or greater than sum of size of two side textures
		R_ASSERT(back_height > 0);
	}

	// Now resize back texture
	float rem;
	int tile;

	float b_width		= elements[flBack].GetOriginalRect().width();
	float b_height		= elements[flBack].GetOriginalRect().height();

	if (bHorizontalOrientation)
	{
		rem			= fmod( back_width, b_width);
		tile		= iFloor(back_width / b_width);	
		elements[flBack].SetPos(iPos.x + f_width, iPos.y);
		elements[flBack].SetTile(tile, 1, rem, 0);
	}
	else
	{
		rem			= fmod(back_height, b_height);
		tile		= iFloor(back_height/b_height);
		elements[flBack].SetPos(iPos.x, iPos.y + f_height);
		elements[flBack].SetTile(1, tile, 0, rem);
	}

	uFlags |= flValidSize;
}

void CUIFrameLine::SetElementsRect( CUIStaticItem& item, int idx )
{
	float srtch_width  = item.GetOriginalRect().width();
	float srtch_height = item.GetOriginalRect().height();

	if ( bStretchTexture )
	{
		VERIFY( (m_parent_wnd_size.x > 0.0f) && (m_parent_wnd_size.y > 0.0f) );
		if ( bHorizontalOrientation )
		{
			srtch_height = m_parent_wnd_size.y;
		}
		else
		{
			srtch_width  = m_parent_wnd_size.x;
		}
	}

	if( bHorizontalOrientation && (idx==flSecond) && UI()->is_16_9_mode() )
		srtch_width			/= 1.2f;

	item.SetRect( Frect().set( 0.0f, 0.0f, srtch_width, srtch_height ) );
}

void CUIFrameLine::Render()
{
	// If size changed - update size
	if ( !(uFlags & flValidSize) )
	{
		UpdateSize();
	}
	// Now render all statics
	for (int i = 0; i < flMax; ++i)
	{
		SetElementsRect( elements[i], i );
		elements[i].Render();
	}
}