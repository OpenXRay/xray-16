/*
 *  Progressive Mesh type Polygon Reduction Algorithm
 *  by Stan Melax (c) 1998
 *  by Oles Shishkovtsov (c) 1999-2000
 *
 */

#ifndef PROGRESSIVE_MESH_H
#define PROGRESSIVE_MESH_H

#ifdef XRPROGRESSIVE_EXPORTS
#define PM_API __declspec(dllexport)
#else
#define PM_API __declspec(dllimport)
#endif

#ifndef IC
#define IC __forceinline
#endif

// ************************* P_UV
#pragma pack(push,1)
class PM_API P_UV {
public:
	float u,v;

	IC float dist(P_UV &p)	{ return _sqrt((u-p.u)*(u-p.u) + (v-p.v)*(v-p.v)); }

};
#pragma pack(pop)

// ************************* VSplit structure
#pragma pack(push,1)
struct PM_API Vsplit
{
	WORD	vsplitVert;		// the vert which I'm splitting off of;
							// (the vert I create is just the current end of the VB)
	BYTE	numNewTriangles;
	BYTE	numFixFaces;
							// the index here is an offset in the FixFaces list
							// which contains offsets in the triangle list
};
struct PM_API PM_Result
{
	WORD*	permutePTR;		// You need to permute vertices according to this
	u32	permuteSIZE;
	Vsplit*	splitPTR;		// Ready to use Vsplit records
	u32	splitSIZE;
	WORD*	facefixPTR;		// Ready to use FaceFix records
	u32	facefixSIZE;

	u32	minVertices;
};
#pragma pack(pop)

// ************************* Main function
extern "C" {
// FIRST:
//			Call following function and...
//			...then create all of your vertices preserving their order
PM_API void	__cdecl PM_Init	(
	u32	dwRelevantUV, u32 dwRelevantUVMASK,
	u32	dwMinVertCount,
	float	w_UV=0.5f,
	float	w_Pos=1.f,
	float	w_Norm=1.f,
	float	p_BorderHeuristic=150.f,
	float	p_BorderHeuristicD=0.07f,
	float	p_QualityHeuristic=0.85f
	);

PM_API void __cdecl PM_CreateVertex(float x, float y, float z,int _id, P_UV *uv);

// SECOND:
//			You need to call this function passing indices (which will be reordered)
//			...and collaped, so initial model state has minimal number of vertices
PM_API int __cdecl PM_Convert(
	WORD*			pIndices,
	u32			idxCount,
	PM_Result*		RESULT
	);

// THIRD:
//		if (res>=0)	Permute & Enjoy! :)
//		else		progressive convert failed
}

#endif
