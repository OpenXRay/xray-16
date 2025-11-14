#include "common.h"

v2p_dumb main ( v_dumb I )
{
	v2p_dumb O;

	O.HPos = mul( m_WVP, I.P );

 	return O;
}
FXVS;
