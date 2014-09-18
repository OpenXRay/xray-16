#pragma once

const u32 BONE_NONE =	0xffffffff;

struct SVertexData{
	u32			bone;
	float		weight;
	SVertexData	(u32 b, float w):bone(b),weight(w){;}
};
DEFINE_VECTOR(SVertexData,VDVec,VDIt);

class CVertexDef
{
public:
	Fvector		P;
	VDVec		data;
public:
				CVertexDef	()			{ZeroMemory(this,sizeof(this));}
	void		SetPosition	(Point3 &p)	{P.set(p.x,p.z,p.y);}
	void		Append		(u32 bone, float weight)	
	{
		data.push_back(SVertexData(bone,weight));
	}
};
DEFINE_VECTOR(CVertexDef*,VertexDefVec,VertexDefIt);

struct st_VERT {
	Fvector			P;
//	u32				sm_group;
	Fvector2		uv;
	VDVec			data;
public:
	st_VERT()
	{
		P.set	(0,0,0);
		uv.set	(0.f,0.f);
	}
	void			Set			(const CVertexDef& D)
	{
		R_ASSERT	(data.empty());
		data		= D.data;
	}

	void			SetUV(float _u, float _v)
	{
		uv.x = _u; uv.y = _v;
	}
	BOOL			similar(const st_VERT& V) const
	{
		if (data.size()!=V.data.size())		return FALSE;
		if (!uv.similar	(V.uv))				return FALSE;
		if (!P.similar	(V.P))				return FALSE;
//		if ((0==sm_group)||(0==(sm_group&V.sm_group))) 		return FALSE;
		for (u32 k=0; k<data.size(); k++){
			if (data[k].bone!=V.data[k].bone)				return FALSE;
			if (!fsimilar(data[k].weight,V.data[k].weight))	return FALSE;
		}
		return TRUE;
	}
};

DEFINE_VECTOR(st_VERT*,ExpVertVec,ExpVertIt);

