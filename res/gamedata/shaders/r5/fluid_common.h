#define LEFTCELL    float3 (input.LR.x, input.texcoords.y, input.texcoords.z)
#define RIGHTCELL   float3 (input.LR.y, input.texcoords.y, input.texcoords.z)
#define BOTTOMCELL  float3 (input.texcoords.x, input.BT.x, input.texcoords.z)
#define TOPCELL     float3 (input.texcoords.x, input.BT.y, input.texcoords.z)
#define DOWNCELL    float3 (input.texcoords.x, input.texcoords.y, input.DU.x)
#define UPCELL      float3 (input.texcoords.x, input.texcoords.y, input.DU.y)

#define	BOX_EXPANSION	1.73

Texture3D	Texture_velocity0;
Texture3D	Texture_velocity1;
Texture3D	Texture_color;
Texture3D	Texture_obstacles;
Texture3D	Texture_obstvelocity;
Texture3D	Texture_pressure;
Texture3D	Texture_tempscalar;
Texture3D	Texture_tempvector;

sampler	samPointClamp;
sampler	samLinear;

cbuffer FluidSimConfig
{
	float	textureHeight;
	float	textureWidth;
	float	textureDepth;

float	modulate = 1.0;
	float	epsilon;
	float	timestep;
float	forward = 1.0;
	float4	floatVolumeDim;
}

cbuffer AABBBounds
{
	float4	boxLBDcorner;
	float4	boxRTUcorner;
}

cbuffer EmitterParams
{
	float	size;
	float4	center;
	float4	splatColor;
}

cbuffer OOBBClipPlanes
{
	float4	OOBBClipPlane[6];
}

static const float k = 0.006;

struct	v_fluidsim
{
	float3	position			: POSITION;
	float3	textureCoords0		: TEXCOORD;
};

struct v2g_fluidsim
{
	float3 cell0				: TEXCOORD0;
	float3 texcoords			: TEXCOORD1;
	float2 LR					: TEXCOORD2;
	float2 BT					: TEXCOORD3;
	float2 DU					: TEXCOORD4;
	float4 pos					: SV_Position;
};

struct g2p_fluidsim
{
	float3	cell0				: TEXCOORD0;
	float3	texcoords			: TEXCOORD1;
	float2	LR					: TEXCOORD2;
	float2	BT					: TEXCOORD3;
	float2	DU					: TEXCOORD4;
	float4	pos					: SV_Position;
	uint	RTIndex				: SV_RenderTargetArrayIndex;
};

struct p_fluidsim
{
	float3	cell0				: TEXCOORD0;
	float3	texcoords			: TEXCOORD1;
	float2	LR					: TEXCOORD2;
	float2	BT					: TEXCOORD3;
	float2	DU					: TEXCOORD4;
};

struct v2g_fluidsim_clip
{
	float3	cell0				: TEXCOORD0;
	float3	texcoords			: TEXCOORD1;
	float2	LR					: TEXCOORD2;
	float2	BT					: TEXCOORD3;
	float2	DU					: TEXCOORD4;
	float3	clip0				: SV_ClipDistance0;
	float3	clip1				: SV_ClipDistance1;
	float4	pos					: SV_Position;
};

struct g2p_fluidsim_clip
{
	float3	cell0				: TEXCOORD0;
	float3	texcoords			: TEXCOORD1;
	float2	LR					: TEXCOORD2;
	float2	BT					: TEXCOORD3;
	float2	DU					: TEXCOORD4;
	float3	clip0				: SV_ClipDistance0;
	float3	clip1				: SV_ClipDistance1;
	float4	pos					: SV_Position;
	uint	RTIndex				: SV_RenderTargetArrayIndex;
};

struct p_fluidsim_clip
{
	float3	cell0				: TEXCOORD0;
	float3	texcoords			: TEXCOORD1;
	float2	LR					: TEXCOORD2;
	float2	BT					: TEXCOORD3;
	float2	DU					: TEXCOORD4;
	float3	clip0				: SV_ClipDistance0;
	float3	clip1				: SV_ClipDistance1;
};

struct v2g_fluidsim_dyn_aabb
{
	float3	cell0				: TEXCOORD0;
	float3	velocity			: TEXCOORD1;
	float3	clip0				: SV_ClipDistance0;
	float3	clip1				: SV_ClipDistance1;
	float4	pos					: SV_Position;
};

struct g2p_fluidsim_dyn_aabb
{
	float3	cell0				: TEXCOORD0;
	float3	velocity			: TEXCOORD1;
	float3	clip0				: SV_ClipDistance0;
	float3	clip1				: SV_ClipDistance1;
	float4	pos					: SV_Position;
	uint	RTIndex				: SV_RenderTargetArrayIndex;
};

struct p_fluidsim_dyn_aabb
{
	float3	cell0				: TEXCOORD0;
	float3	velocity			: TEXCOORD1;
	float3	clip0				: SV_ClipDistance0;
	float3	clip1				: SV_ClipDistance1;
};

float4 GetObstVelocity( float3 cellTexCoords )
{
    return Texture_obstvelocity.SampleLevel(samPointClamp, cellTexCoords, 0);
}

bool IsNonEmptyCell( float3 cellTexCoords )
{
    return (Texture_obstacles.SampleLevel(samPointClamp, cellTexCoords, 0).r > 0.0);
}

bool IsBoundaryCell( float3 cellTexCoords )
{
    return (Texture_obstacles.SampleLevel(samPointClamp, cellTexCoords, 0).r > 0.9);    
}

float3 GetAdvectedPosTexCoords(p_fluidsim input)
{
    float3 pos = input.cell0;

    pos -= timestep * forward *
        Texture_velocity0.SampleLevel( samPointClamp, input.texcoords, 0 ).xyz;

    return float3(pos.x/textureWidth, pos.y/textureHeight, (pos.z+0.5)/textureDepth);
}