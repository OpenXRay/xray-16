#include "common.h"
#include "iostructs\v_TL2uv.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Vertex
v2p_TL2uv _main ( v_TL2uv I )
{
	v2p_TL2uv	O;

	O.HPos = I.P;
	O.Tex0 = I.Tex0;
	O.Tex1 = I.Tex1;
	//	Some shaders that use this stub don't need Color at all
	O.Color = unpack_D3DCOLOR(I.Color);

 	return O;
}