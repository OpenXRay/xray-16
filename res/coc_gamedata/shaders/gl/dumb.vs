#include "common.h"
#include "iostructs\v_dumb.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Vertex
v2p_dumb _main ( v_dumb I )
{
	v2p_dumb O;

	O.HPos = mul( m_WVP, I.P );

 	return O;
}
