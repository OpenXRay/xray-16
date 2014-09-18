// Blender_Tree.h: interface for the CBlender_Tree class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BLENDER_VERTEX_AREF_H__07141B30_A968_407E_86F8_D12702FE0B9B__INCLUDED_3)
#define AFX_BLENDER_VERTEX_AREF_H__07141B30_A968_407E_86F8_D12702FE0B9B__INCLUDED_3
#pragma once

class CBlender_Tree : public IBlender  
{
public:
	xrP_BOOL	oBlend;
	xrP_BOOL	oNotAnTree;
public:
	virtual		LPCSTR		getComment()	{ return "LEVEL: trees/bushes";	}
	virtual		BOOL		canBeLMAPped()	{ return FALSE; }
	virtual		BOOL		canBeDetailed()	{ return TRUE; }

	virtual		void		Save			(IWriter&	fs);
	virtual		void		Load			(IReader&	fs, u16 version);

	virtual		void		Compile			(CBlender_Compile& C);

	CBlender_Tree();
	virtual ~CBlender_Tree();
};

#endif // !defined(AFX_BLENDER_VERTEX_AREF_H__07141B30_A968_407E_86F8_D12702FE0B9B__INCLUDED_3)
