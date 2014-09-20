// BlenderDefault.h: interface for the CBlenderDefault class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BLENDERDEFAULT_H__C12F64EE_43E7_4483_9AC3_29272E0401E7__INCLUDED_1)
#define AFX_BLENDERDEFAULT_H__C12F64EE_43E7_4483_9AC3_29272E0401E7__INCLUDED_1
#pragma once

class CBlender_LaEmB : public IBlender  
{
public:
	string64	oT2_Name;		// name of secondary texture
	string64	oT2_xform;		// xform for secondary texture
	string64	oT2_const;
	
	void		compile_ED	(CBlender_Compile& C);
	void		compile_EDc	(CBlender_Compile& C);
	void		compile_L	(CBlender_Compile& C);
	void		compile_Lc	(CBlender_Compile& C);
	void		compile_2	(CBlender_Compile& C);
	void		compile_2c	(CBlender_Compile& C);
	void		compile_3	(CBlender_Compile& C);
	void		compile_3c	(CBlender_Compile& C);
public:
	virtual		LPCSTR		getComment()	{ return "LEVEL: (lmap+env*const)*base";	}
	virtual		BOOL		canBeLMAPped()	{ return TRUE; }

	virtual		void		Save			(IWriter&  fs);
	virtual		void		Load			(IReader&	fs, u16 version);

	virtual		void		Compile			(CBlender_Compile& C);

	CBlender_LaEmB();
	virtual ~CBlender_LaEmB();
};

#endif // !defined(AFX_BLENDERDEFAULT_H__C12F64EE_43E7_4483_9AC3_29272E0401E7__INCLUDED_1)
