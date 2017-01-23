// Blender_Vertex.h: interface for the CBlender_Vertex class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BLENDER_VERTEX_H__D3B42F77_7018_4672_B6A5_6EE6BD947662__INCLUDED_)
#define AFX_BLENDER_VERTEX_H__D3B42F77_7018_4672_B6A5_6EE6BD947662__INCLUDED_
#pragma once

class CBlender_Vertex : public IBlender  
{
public:
	virtual		LPCSTR		getComment()	{ return "LEVEL: diffuse*base";	}
	virtual		BOOL		canBeDetailed()	{ return TRUE;	}
	virtual		BOOL		canBeLMAPped()	{ return FALSE; }

	virtual		void		Save			(IWriter&  fs);
	virtual		void		Load			(IReader&	fs, u16 version);

	virtual		void		Compile			(CBlender_Compile& C);

	CBlender_Vertex();
	virtual ~CBlender_Vertex();

private:
	xrP_TOKEN	oTessellation;
};

#endif // !defined(AFX_BLENDER_VERTEX_H__D3B42F77_7018_4672_B6A5_6EE6BD947662__INCLUDED_)
