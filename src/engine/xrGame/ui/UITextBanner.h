//=============================================================================
//  Красивый текстовый баннер с множеством стилей анимации
//=============================================================================

#ifndef UI_TEXT_BANNER_H_
#define UI_TEXT_BANNER_H_

// #pragma once;

#include "UIStatic.h"

//-----------------------------------------------------------------------------/
//  Класс параметров эффекта
//-----------------------------------------------------------------------------/

struct EffectParams
{
	friend class	CUITextBanner;
	float			fPeriod;
	bool			bCyclic;
	bool			bOn;
	int				iAdditionalParam;
	float			fAdditionalParam;
	int				iEffectStage;

	// Constructor
	EffectParams()
		:	fPeriod				(0.0f),
			fTimePassed			(0.0f),
			iEffectStage		(0),
			fAdditionalParam	(0.0f),
			iAdditionalParam	(0),
			bCyclic				(true),
			bOn					(true)
	{}
private:
	float			fTimePassed;
};

//-----------------------------------------------------------------------------/
//  Класс анимироанного баннера
//-----------------------------------------------------------------------------/

class CUITextBanner
{
public:
	enum TextBannerStyles
	{
		tbsNone		= 0,
		tbsFlicker	= 1,
		tbsFade		= 1 << 1
	};
	// Ctor and Dtor
	CUITextBanner				();
	~CUITextBanner				();

	virtual void	Update		();
	void			Out			(float x, float y, const char *fmt, ...);

	// Установить параметры визуализации баннера. Флаги см. перечисление TextBannerStyles
	EffectParams * SetStyleParams(const TextBannerStyles styleName);
	void		ResetAnimation		(const TextBannerStyles styleName);

	// Font
	void		SetFont				(CGameFont *pFont)	{ m_pFont = pFont; }
	CGameFont	*GetFont			() const { return m_pFont; }
	void		SetFontSize			(float sz)	{ fontSize = sz; }
	void		SetFontAlignment	(CGameFont::EAligment al) {aligment = al;}

	// Color
	void		SetTextColor		(u32 cl);
	u32			GetTextColor		();

	// Вкл/выкл анимации
	void		PlayAnimation		()					{ m_bAnimate = true;	}
	void		StopAnimation		()					{ m_bAnimate = false;	}

protected:
	// Переменные времени для каждого из стилей.
	// В паре:	first	- контрольный период эффекта (задаваемый пользователем)
	//			second	- прошедшее время с текущего апдейта
	typedef xr_map<TextBannerStyles, EffectParams>		StyleParams;
	typedef StyleParams::iterator						StyleParams_it;
	StyleParams											m_StyleParams;

	// Процедуры изменения параметров надписи для эффектов
	void EffectFade();
	void EffectFlicker();

	// Флаг, который определяет состояние анимации
	bool		m_bAnimate;

	// Font
	CGameFont				*m_pFont;
	float					fontSize;
	CGameFont::EAligment	aligment;

	// Letters color
	u32			m_Cl;

};

#endif
