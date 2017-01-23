#pragma once

#include "uistaticitem.h"
#include "ui/uiabstract.h"

class CUIFrameRect: public CUISimpleWindow, CUIMultiTextureOwner
{
public:
	enum EFramePart{
		fmBK=0,
		fmL, fmR, fmT, fmB, fmLT, fmRB, fmRT, fmLB, fmMax
	};
	Flags16		m_itm_mask;

	friend class CUIFrameWindow;

						CUIFrameRect	();
	virtual void		InitTexture		(LPCSTR texture);
	virtual void		InitTextureEx	(LPCSTR texture, LPCSTR  shader);

	virtual void		Draw			();
	virtual void		Draw			(float x, float y);
	virtual void		SetWndPos		(const Fvector2& pos);
	virtual void		SetWndSize		(const Fvector2& size);
	virtual void		SetWndRect		(const Frect& rect);
	virtual void		SetWidth		(float width);
	virtual void		SetHeight		(float height);
	virtual void		Update			();
			void		SetTextureColor	(u32 cl);
			void		SetVisiblePart	(EFramePart p, BOOL b)	{m_itm_mask.set(u16(1<<p), b);};
protected:
	CUIStaticItem	frame[fmMax];

	enum {
		flValidSize	= (1<<0),
		flSingleTex	= (1<<1),
	};
	Flags8			uFlags;
	void			UpdateSize		( bool recall = false );
};