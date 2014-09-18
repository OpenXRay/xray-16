// SkeletonX.h: interface for the CSkeletonX class.
//
//////////////////////////////////////////////////////////////////////

#ifndef		FSkinnedH
#define		FSkinnedH
#pragma		once

#include	"FVisual.h"
#include	"FProgressive.h"
#ifdef _EDITOR
#	include	"SkeletonX.h"
#else
#	include	"SkeletonX.h"
#endif
struct	SEnumVerticesCallback;
class		CSkeletonX_ext	: public CSkeletonX	// shared code for SkeletonX derivates
{
protected:
	virtual void			_Load_hw			(Fvisual& V,		void *		data);
	virtual void			_CollectBoneFaces	(Fvisual* V,		u32 iBase,	u32 iCount);
			void			_EnumBoneVertices	( SEnumVerticesCallback &C, Fvisual* V, u16 bone_id, u32 iBase, u32 iCount ) const;
	virtual void			_FillVerticesHW1W	(const Fmatrix& view, CSkeletonWallmark& wm, const Fvector& normal, float size, Fvisual* V, u16* indices, CBoneData::FacesVec& faces);
	virtual void			_FillVerticesHW2W	(const Fmatrix& view, CSkeletonWallmark& wm, const Fvector& normal, float size, Fvisual* V, u16* indices, CBoneData::FacesVec& faces);
	virtual void			_FillVerticesHW3W	(const Fmatrix& view, CSkeletonWallmark& wm, const Fvector& normal, float size, Fvisual* V, u16* indices, CBoneData::FacesVec& faces);
	virtual void			_FillVerticesHW4W	(const Fmatrix& view, CSkeletonWallmark& wm, const Fvector& normal, float size, Fvisual* V, u16* indices, CBoneData::FacesVec& faces);
	virtual void			_FillVertices		(const Fmatrix& view, CSkeletonWallmark& wm, const Fvector& normal, float size, Fvisual* V, u16 bone_id, u32 iBase, u32 iCount);

	virtual BOOL			_PickBoneHW1W		(IKinematics::pick_result &r, float range, const Fvector& S, const Fvector& D, Fvisual* V, u16* indices, CBoneData::FacesVec& faces);
	virtual BOOL			_PickBoneHW2W		(IKinematics::pick_result &r, float range, const Fvector& S, const Fvector& D, Fvisual* V, u16* indices, CBoneData::FacesVec& faces);
	virtual BOOL			_PickBoneHW3W		(IKinematics::pick_result &r, float range, const Fvector& S, const Fvector& D, Fvisual* V, u16* indices, CBoneData::FacesVec& faces);
	virtual BOOL			_PickBoneHW4W		(IKinematics::pick_result &r, float range, const Fvector& S, const Fvector& D, Fvisual* V, u16* indices, CBoneData::FacesVec& faces);

	virtual BOOL			_PickBone			(IKinematics::pick_result &r, float range, const Fvector& S, const Fvector& D, Fvisual* V, u16 bone_id, u32 iBase, u32 iCount);

public:
};

class CSkeletonX_ST			: public Fvisual, public CSkeletonX_ext
{
private:
	typedef Fvisual			inherited1;
	typedef CSkeletonX_ext	inherited2;
public:
							CSkeletonX_ST	()	{}
	virtual					~CSkeletonX_ST	()	{}
	virtual void			Render			(float LOD);
	virtual void			Load			(const char* N, IReader *data, u32 dwFlags);
	virtual void			Copy			(dxRender_Visual *pFrom);
	virtual void			Release			();
	virtual void			AfterLoad		(CKinematics* parent, u16 child_idx);
	virtual void			EnumBoneVertices(SEnumVerticesCallback &C, u16 bone_id);
	virtual BOOL			PickBone		(IKinematics::pick_result &r, float dist, const Fvector& start, const Fvector& dir, u16 bone_id);
	virtual void			FillVertices	(const Fmatrix& view, CSkeletonWallmark& wm, const Fvector& normal, float size, u16 bone_id);
private:
	CSkeletonX_ST				(const CSkeletonX_ST& other);
	void	operator=			( const CSkeletonX_ST& other);
};

class CSkeletonX_PM			: public FProgressive, public CSkeletonX_ext
{
private:
	typedef FProgressive	inherited1;
	typedef CSkeletonX_ext	inherited2;
public:
							CSkeletonX_PM	()	{}
	virtual					~CSkeletonX_PM	()	{}
	virtual void			Render			(float LOD);
	virtual void			Load			(const char* N, IReader *data, u32 dwFlags);
	virtual void			Copy			(dxRender_Visual *pFrom);
	virtual void			Release			();
	virtual void			AfterLoad		(CKinematics* parent, u16 child_idx);
	virtual void			EnumBoneVertices(SEnumVerticesCallback &C, u16 bone_id);
	virtual BOOL			PickBone		(IKinematics::pick_result &r, float dist, const Fvector& start, const Fvector& dir, u16 bone_id);
	virtual void			FillVertices	(const Fmatrix& view, CSkeletonWallmark& wm, const Fvector& normal, float size, u16 bone_id);
private:
	CSkeletonX_PM				(const CSkeletonX_PM& other);
	void	operator=			( const CSkeletonX_PM& other);
};

#endif // FSkinnedH
