#ifndef	UI_ARROW_H_INCLUDED
#define UI_ARROW_H_INCLUDED

#include "UIStatic.h"

class UI_Arrow: public CUIStatic
{
private:
	typedef CUIStatic	inherited;

public:
					UI_Arrow		();
	virtual			~UI_Arrow		();

			void	init_from_xml	( CUIXml& xml, LPCSTR path, CUIWindow* parent );
			void	SetNewValue		( float new_value );
			void	SetPos			( float pos );
	IC		float	GetPos			()	{	return m_pos;	}

private:
	float		m_angle_begin;
	float		m_angle_end;
	float		m_ang_velocity;
	float		m_angle_range;

	float		m_temp_pos;
	float		m_pos;

}; // class UI_Arrow

#endif // UI_ARROW_H_INCLUDED
