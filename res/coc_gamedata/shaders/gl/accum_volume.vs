#include "common.h"
#include "iostructs\v_volume.h"

//////////////////////////////////////////////////////////////////////////////////////////
uniform float4x4	m_texgen;
#ifdef	USE_SJITTER
uniform float4x4	m_texgen_J;
#endif

//////////////////////////////////////////////////////////////////////////////////////////
// Vertex
v2p_volume _main ( float4 P )
{
	v2p_volume	O;
	O.hpos 		= mul( m_WVP, P );
	O.tc 		= mul( m_texgen, P );
#ifdef	USE_SJITTER
	O.tcJ 		= mul( m_texgen_J, P );
#endif
 	return	O;
}
