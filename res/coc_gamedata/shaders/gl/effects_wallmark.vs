#include "common.h"
#include "iostructs\v_TL.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Vertex
v2p_TL _main ( v_TL I )
{
	v2p_TL	O;

	O.HPos		= mul( m_VP, I.P );
	O.Tex0		= I.Tex0;
	O.Color		= unpack_D3DCOLOR(I.Color);

 	return	O;
}