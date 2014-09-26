// Blender_default_aref.h: interface for the CBlender_default_aref class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BLENDER_DEFAULT_AREF_H__E17F011F_C371_4464_B75A_01D68F55FC4E__INCLUDED_)
#define AFX_BLENDER_DEFAULT_AREF_H__E17F011F_C371_4464_B75A_01D68F55FC4E__INCLUDED_
#pragma once

class CBlender_default_aref : public IBlender  
{
public:
	xrP_Integer	oAREF;
	xrP_BOOL	oBlend;
public:
	virtual		LPCSTR		getComment()	{ return "LEVEL: lmap*base.aref";	}
	virtual		BOOL		canBeDetailed()	{ return TRUE; }
	virtual		BOOL		canBeLMAPped()	{ return TRUE; }

	virtual		void		Save			(IWriter&	fs);
	virtual		void		Load			(IReader&	fs, u16 version);

	virtual		void		Compile			(CBlender_Compile& C);

	CBlender_default_aref();
	virtual ~CBlender_default_aref();
};

#endif // !defined(AFX_BLENDER_DEFAULT_AREF_H__E17F011F_C371_4464_B75A_01D68F55FC4E__INCLUDED_)
