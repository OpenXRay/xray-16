#include "common.h"
#include "iostructs\v_TL.h"

v2p_TL _main (v_TL I)
{
	v2p_TL	O;

	O.Tex0		= I.Tex0;
	O.HPos		= I.P;
	O.HPos.w	= 1;
	O.HPos		= mul( m_WVP, O.HPos );
	return 		O;
}
