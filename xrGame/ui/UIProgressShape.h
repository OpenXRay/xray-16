#pragma once

#include "UIStatic.h"

class CUIStatic;

class CUIProgressShape : public CUIStatic {
friend class CUIXmlInit;
public:
						CUIProgressShape		();
	virtual				~CUIProgressShape		();
	void				SetPos					(int pos, int max);
	void				SetPos					(float pos);
	void				SetTextVisible			(bool b);

	virtual void		Draw					();

protected:
	bool				m_bClockwise;
	u32					m_sectorCount;
	float				m_stage;
	bool				m_bText;
	bool				m_blend;

	float				m_angle_begin;
	float				m_angle_end;
};