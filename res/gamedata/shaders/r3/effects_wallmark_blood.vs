/**
 * @ Version: SCREEN SPACE SHADERS - UPDATE 17
 * @ Description: Decals - VS
 * @ Modified time: 2023-07-05 05:58
 * @ Author: https://www.moddb.com/members/ascii1457
 * @ Mod: https://www.moddb.com/mods/stalker-anomaly/addons/screen-space-shaders
 */

#include "common.h"

v2p_TL main(v_TL I)
{
	v2p_TL O;

	O.HPos 	= mul(m_VP, I.P);
	O.Tex0 	= I.Tex0;
	O.Color = I.Color.bgra;

	return O;
}