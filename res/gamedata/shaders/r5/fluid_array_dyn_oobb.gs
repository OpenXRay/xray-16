#include "fluid_common.h"

//////////////////////////////////////////////////////////////////////////////////////////
//	Geometry
[maxvertexcount (3)]
void main(triangle v2g_fluidsim_dyn_aabb In[3], inout TriangleStream<g2p_fluidsim_dyn_aabb> triStream)
{
    g2p_fluidsim_dyn_aabb Out;
    // cell0.z of the first vertex in the triangle determines the destination slice index
    Out.RTIndex = In[0].cell0.z;
    for(int v=0; v<3; v++)
    {
        Out.pos			= In[v].pos; 
        Out.cell0		= In[v].cell0;
		Out.velocity	= In[v].velocity;
		Out.clip0		= In[v].clip0;
		Out.clip1		= In[v].clip1;
        triStream.Append( Out );
    }
    triStream.RestartStrip( );
}