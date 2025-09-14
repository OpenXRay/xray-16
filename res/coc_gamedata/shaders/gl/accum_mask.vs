#include "common.h"

out gl_PerVertex { vec4 gl_Position; };

layout(location = POSITION) in float4 P;

void main ()
{
	gl_Position = mul ( m_WVP, P );
}
