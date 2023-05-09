// SkeletonX.h: interface for the CSkeletonX class.
//
//////////////////////////////////////////////////////////////////////
#pragma once
#ifndef FSkinnedH
#define FSkinnedH

#include "FVisual.h"
#include "FProgressive.h"
#include "SkeletonX.h"
struct SEnumVerticesCallback;
class CSkeletonX_ext : public CSkeletonX // shared code for SkeletonX derivates
{
protected:
    virtual void _Load_hw(Fvisual& V, void* data);

    virtual void _CollectBoneFaces(Fvisual* V, u32 iBase, u32 iCount);

    void _EnumBoneVertices(SEnumVerticesCallback& C, Fvisual* V, u16 bone_id, u32 iBase, u32 iCount) const;

private:
    template <typename T>
    void _CollectBoneFacesHW(u16* indices, Fvisual* V, u32 iCount);

    template <typename T>
    void _FillVerticesHW(const Fmatrix& view, CSkeletonWallmark& wm, const Fvector& normal, float size,
        Fvisual* V, u16* indices, CBoneData::FacesVec& faces);

protected:
    virtual void _FillVertices(const Fmatrix& view, CSkeletonWallmark& wm, const Fvector& normal, float size,
        Fvisual* V, u16 bone_id, u32 iBase, u32 iCount);

    virtual BOOL _PickBone(IKinematics::pick_result& r, float range, const Fvector& S, const Fvector& D, Fvisual* V,
        u16 bone_id, u32 iBase, u32 iCount);
};

class CSkeletonX_ST : public Fvisual, public CSkeletonX_ext
{
    typedef Fvisual inherited1;
    typedef CSkeletonX_ext inherited2;

public:
    CSkeletonX_ST() {}
    virtual ~CSkeletonX_ST() {}
    virtual void Render(CBackend& cmd_list, float LOD, bool use_fast_geo) override;
    virtual void Load(const char* N, IReader* data, u32 dwFlags);
    virtual void Copy(dxRender_Visual* pFrom);
    virtual void Release();
    virtual void AfterLoad(CKinematics* parent, u16 child_idx);
    virtual void EnumBoneVertices(SEnumVerticesCallback& C, u16 bone_id);
    virtual BOOL PickBone(
        IKinematics::pick_result& r, float dist, const Fvector& start, const Fvector& dir, u16 bone_id);
    virtual void FillVertices(
        const Fmatrix& view, CSkeletonWallmark& wm, const Fvector& normal, float size, u16 bone_id);

private:
    CSkeletonX_ST(const CSkeletonX_ST& other);
    void operator=(const CSkeletonX_ST& other);
};

class CSkeletonX_PM : public FProgressive, public CSkeletonX_ext
{
    typedef FProgressive inherited1;
    typedef CSkeletonX_ext inherited2;

public:
    CSkeletonX_PM() {}
    virtual ~CSkeletonX_PM() {}
    virtual void Render(CBackend& cmd_list, float LOD, bool use_fast_geo) override;
    virtual void Load(const char* N, IReader* data, u32 dwFlags);
    virtual void Copy(dxRender_Visual* pFrom);
    virtual void Release();
    virtual void AfterLoad(CKinematics* parent, u16 child_idx);
    virtual void EnumBoneVertices(SEnumVerticesCallback& C, u16 bone_id);
    virtual BOOL PickBone(
        IKinematics::pick_result& r, float dist, const Fvector& start, const Fvector& dir, u16 bone_id);
    virtual void FillVertices(
        const Fmatrix& view, CSkeletonWallmark& wm, const Fvector& normal, float size, u16 bone_id);

private:
    CSkeletonX_PM(const CSkeletonX_PM& other);
    void operator=(const CSkeletonX_PM& other);
};

#endif // FSkinnedH
