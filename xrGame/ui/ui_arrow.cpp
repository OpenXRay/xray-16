
#include "stdafx.h"

#include "ui_arrow.h"
#include "UIXmlInit.h"


UI_Arrow::UI_Arrow()
{
	m_angle_begin	= 0.0f;
	m_angle_end		= PI_MUL_2;
	m_ang_velocity	= 1.0f;
	m_angle_range	= 0.0f;
	m_temp_pos		= 0.0f;
	m_pos			= 0.0f;
}

UI_Arrow::~UI_Arrow()
{
}

void UI_Arrow::init_from_xml( CUIXml& xml, LPCSTR path, CUIWindow* parent )
{
	//m_arrow             = UIHelper::CreateStatic( xml, "arrow", this );
	parent->AttachChild( this );
	SetAutoDelete( true );
	CUIXmlInit::InitStatic( xml, path, 0, this );

	m_angle_begin  = xml.ReadAttribFlt( path, 0, "begin_angle", 0.0f     );
	m_angle_end    = xml.ReadAttribFlt( path, 0, "end_angle",   PI_MUL_2 );
	m_ang_velocity = xml.ReadAttribFlt( path, 0, "ang_velocity", 1.0f );
	bool arrow_clockwise = ( xml.ReadAttribInt( path, 0, "clockwise", 1 ) == 1 )? true : false;
	if ( arrow_clockwise )
	{
		m_angle_range = -_abs( m_angle_end - m_angle_begin );
	}
	else
	{
		m_angle_range = _abs( m_angle_end - m_angle_begin );
	}

}

void UI_Arrow::SetNewValue( float new_value )
{
	clamp( new_value, 0.0f, 1.0f );
	if ( fsimilar( m_pos, m_temp_pos ) )
	{
		m_temp_pos = m_pos + 1.05f * ( new_value - m_pos );
		clamp( m_temp_pos, 0.0f, 1.0f );
	}
	else
	{
		float dif = m_temp_pos - m_pos;
		float val = m_ang_velocity * Device.fTimeDelta;

		val	= _min( _abs(val), _abs(dif) );
		val	*= (dif > 0.0f)? +1.0f : -1.0f;
		m_pos += val;
	}
	clamp( m_pos, 0.0f, 1.0f ); // min = 0,  max = 1
	SetPos( m_pos );
}

void UI_Arrow::SetPos( float pos )
{
	m_pos = pos;
	inherited::SetHeading( m_angle_begin + m_pos * m_angle_range );
}
