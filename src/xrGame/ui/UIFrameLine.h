#pragma once

#include "../UIStaticItem.h"

class CUIFrameLine: public CUICustomItem
{
	friend class CUIFrameLineWnd;
	enum
	{
		flFirst = 0,	// Left or top
		flSecond,		// Right or bottom
		flBack,			// Center texture
		flMax
	};

	// Drawing elements
	CUIStaticItem	elements[flMax];

	enum
	{
		flValidSize = 1
	};

public:
	bool		bStretchTexture;

protected:
	float		iSize;
	Fvector2	iPos;
	u8			uFlags;
	bool		bHorizontalOrientation;
	Fvector2	m_parent_wnd_size;

	void		UpdateSize		();
public:
				CUIFrameLine	();

	void		set_parent_wnd_size(Fvector2 const& size)	{ m_parent_wnd_size = size; }

	void		InitFrameLine	(Fvector2 pos, float size, bool horizontal, DWORD align);
	void		InitTexture		(LPCSTR texture, LPCSTR sh_name);
	void		SetColor		(u32 cl);
	IC void		SetPos			(Fvector2 pos)				{ iPos.set(pos);	uFlags &=~ flValidSize; }
	IC void		SetSize			(float size)				{ iSize = size;			uFlags &=~ flValidSize; }
	IC void		SetOrientation	(bool bIsHorizontal)		{ bHorizontalOrientation = bIsHorizontal; uFlags &=~ flValidSize; }
	void		SetElementsRect	(CUIStaticItem& item,int idx);
	void		Render			();
};