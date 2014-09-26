#ifndef fmeshH
#define fmeshH
#pragma once

//BOOL ValidateIndices		(u32 vCount, u32 iCount, u16* pIndices);

///////////////////////////////////////////////////////////////////////////////////////////////////////
// MESH as it is represented in file
enum MT {
	MT_NORMAL				=0,
	MT_HIERRARHY			=1,
	MT_PROGRESSIVE			=2,
	MT_SKELETON_ANIM		=3,
	MT_SKELETON_GEOMDEF_PM	=4,
	MT_SKELETON_GEOMDEF_ST	=5,
	MT_LOD					=6,
	MT_TREE_ST				=7,
	MT_PARTICLE_EFFECT		=8,
	MT_PARTICLE_GROUP		=9,
	MT_SKELETON_RIGID		=10,
	MT_TREE_PM				=11,

	MT_3DFLUIDVOLUME		=12,
};

enum OGF_Chuncks {
	OGF_HEADER				= 1,
	OGF_TEXTURE				= 2,
	OGF_VERTICES			= 3,
	OGF_INDICES				= 4,
	OGF_P_MAP				= 5,	//---------------------- unused
	OGF_SWIDATA				= 6,
	OGF_VCONTAINER			= 7, // not used ??
	OGF_ICONTAINER			= 8, // not used ??
    OGF_CHILDREN			= 9,	// * For skeletons only
	OGF_CHILDREN_L			= 10,	// Link to child visuals
	OGF_LODDEF2				= 11,	// + 5 channel data
	OGF_TREEDEF2			= 12,	// + 5 channel data
	OGF_S_BONE_NAMES		= 13,	// * For skeletons only
	OGF_S_MOTIONS 			= 14,	// * For skeletons only
	OGF_S_SMPARAMS  		= 15,	// * For skeletons only
	OGF_S_IKDATA			= 16,	// * For skeletons only
	OGF_S_USERDATA			= 17,	// * For skeletons only (Ini-file)
	OGF_S_DESC				= 18,	// * For skeletons only
	OGF_S_MOTION_REFS		= 19,	// * For skeletons only
	OGF_SWICONTAINER		= 20,	// * SlidingWindowItem record container
	OGF_GCONTAINER			= 21,	// * both VB&IB
	OGF_FASTPATH			= 22,	// * extended/fast geometry
	OGF_S_LODS 				= 23,	// * For skeletons only (Ini-file)
	OGF_S_MOTION_REFS2		= 24,	// * changes in format
	OGF_COLLISION_VERTICES	= 25,
	OGF_COLLISION_INDICES	= 26,
    OGF_forcedword			= 0xFFFFFFFF         
};							

enum OGF_SkeletonVertType	{
	OGF_VERTEXFORMAT_FVF_1L	= 1*0x12071980,
	OGF_VERTEXFORMAT_FVF_2L	= 2*0x12071980,
	OGF_VERTEXFORMAT_FVF_3L	= 4*0x12071980,
	OGF_VERTEXFORMAT_FVF_4L	= 5*0x12071980,
	OGF_VERTEXFORMAT_FVF_NL	= 3*0x12071980,
};

const u16	xrOGF_SMParamsVersion	= 4;

// OGF_DESC
struct ECORE_API ogf_desc	{
	shared_str	source_file;
    shared_str	build_name;
    time_t		build_time;
    shared_str	create_name;
    time_t		create_time;
    shared_str	modif_name;
    time_t		modif_time;
    		ogf_desc():build_time(0),create_time(0),modif_time(0){}
    void 	Load	(IReader& F);
    void 	Save	(IWriter& F);
};

// OGF_BBOX
struct ogf_bbox		{
	Fvector min;
	Fvector max;
};

// OGF_BSPHERE
struct ogf_bsphere	{
	Fvector c;
	float	r;
};

// OGF_HEADER
const u8	xrOGF_FormatVersion		= 4;
struct ogf_header {
	u8			format_version;			// = xrOGF_FormatVersion
	u8			type;					// MT
	u16			shader_id;				// should not be ZERO
	ogf_bbox	bb;
	ogf_bsphere	bs;
};

// Sliding Window Record
struct ENGINE_API	FSlideWindow		{
	u32				offset;
	u16				num_tris;
	u16				num_verts;
};
struct ENGINE_API	FSlideWindowItem	{
	FSlideWindow*	sw;
	u32				count;
	u32				reserved[4];
	FSlideWindowItem() : sw(0),count(0)	{};
};

// OGF_TEXTURE1
//  Z-String - name
//  Z-String - shader

// OGF_TEXTURE_L
//	u32 T_link
//  u32 S_link

// OGF_MATERIAL
//  Fmaterial

// OGF_CHIELDS_L
//	u32	Count
//	u32	idx[0...Count-1]

// OGF_VERTICES
//	u32		dwVertType;			// Type of vertices it contains
//	u32		dwVertCount;		// Count of vertices
//	..vertices itself

// OGF_INDICES
//	u32		Count
//  ..indices itself (u16[Count]);

// OGF_VCONTAINER
// u32		CID;		// Container ID
// u32		Offset;		// Add for every IDX and use as the start vertex
// u32		Count;		// Number of vertices

// OGF_BONES
// BYTE			Count;
// BYTE			Indices[...]

// OGF_TREEDEF
// xform : matrix4x4
// scale : vec4
// bias  : vec4

#endif // fmeshH
