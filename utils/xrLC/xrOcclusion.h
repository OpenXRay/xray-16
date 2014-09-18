#ifndef _OCCLUSION_
#define _OCCLUSION_

#ifdef XROCCLUSION_EXPORTS
#define ORM_API __declspec(dllexport)
#else
#define ORM_API __declspec(dllimport)
#endif

#ifndef IC
#define IC __forceinline
#endif

#pragma pack(push,1)
#define	ORM_FVF	(D3DFVF_XYZ | D3DFVF_DIFFUSE)
struct ORM_API	ORM_Vertex
{
	float	x,y,z;
	u32	dummycolor;
};
#pragma pack(pop)

ORM_API HRESULT __cdecl	ORM_Create	(BOOL bHW, float fViewFar);
ORM_API HRESULT __cdecl	ORM_Destroy	();

ORM_API ORM_Vertex*	__cdecl	ORM_Begin	(int vCount, int iCount, u16* idx);
ORM_API void		__cdecl	ORM_End		(Fvector &C, float R);

ORM_API void		__cdecl	ORM_Process	(
		u32		Count,
		Fvector&	Pos,		// position of test point
		u16*		ID,			// id's of tested models
		BOOL*		R			// boolean result, TRUE-visible
		);
#endif
