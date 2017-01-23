// Blender_Vertex_aref.h: interface for the CBlender_Vertex_aref class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BLENDER_VERTEX_AREF_H__07141B30_A968_407E_86F8_D12702FE0B9B__INCLUDED_2)
#define AFX_BLENDER_VERTEX_AREF_H__07141B30_A968_407E_86F8_D12702FE0B9B__INCLUDED_2
#pragma once

class CBlender_Detail_Still : public IBlender  
{
public:
	xrP_BOOL	oBlend;
public:
	virtual		LPCSTR		getComment()	{ return "LEVEL: detail objects";	}
	virtual		BOOL		canBeLMAPped()	{ return FALSE; }

	virtual		void		Save			(IWriter&  fs);
	virtual		void		Load			(IReader&	fs, u16 version);

	virtual		void		Compile			(CBlender_Compile& C);

	CBlender_Detail_Still();
	virtual ~CBlender_Detail_Still();
};

#endif // !defined(AFX_BLENDER_VERTEX_AREF_H__07141B30_A968_407E_86F8_D12702FE0B9B__INCLUDED_2)
