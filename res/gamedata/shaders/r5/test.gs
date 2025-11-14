#include "common.h"


//////////////////////////////////////////////////////////////////////////////////////////
// Geometry
[maxvertexcount (3)]
void main(triangle v2p_TL In[3], inout TriangleStream<v2p_TL> triStream)
{
    v2p_TL Out;

    for(int v=0; v<3; v++)
    {
		Out.HPos = In[v].HPos;
		Out.Tex0 = In[v].Tex0;
		Out.Color = float4(0,0,1,1);	//	swizzle vertex colour
        triStream.Append( Out );
    }
    triStream.RestartStrip();
}