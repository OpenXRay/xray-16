// BlenderDefault.h: interface for the CBlenderDefault class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BLENDERDEFAULT_H__C12F64EE_43E7_4483_9AC3_29272E0401E7__INCLUDED_BmmD)
#define AFX_BLENDERDEFAULT_H__C12F64EE_43E7_4483_9AC3_29272E0401E7__INCLUDED_BmmD
#pragma once

class CBlender_BmmD : public IBlender  
{
public:
	string64	oT2_Name;	// name of secondary texture
	string64	oT2_xform;	// xform for secondary texture
	string64	oR_Name	;	//. задел на будущее
	string64	oG_Name	;	//. задел на будущее
	string64	oB_Name	;	//. задел на будущее
	string64	oA_Name	;	//. задел на будущее
public:
	virtual		LPCSTR		getComment()	{ return "LEVEL: Implicit**detail";	}
	virtual		BOOL		canBeDetailed()	{ return TRUE; }
	virtual		BOOL		canBeLMAPped()	{ return TRUE; }

	virtual		void		Save			( IWriter&	fs);
	virtual		void		Load			( IReader&	fs, u16 version);

	virtual		void		Compile			( CBlender_Compile& C);

	CBlender_BmmD();
	virtual ~CBlender_BmmD();
};

#endif // !defined(AFX_BLENDERDEFAULT_H__C12F64EE_43E7_4483_9AC3_29272E0401E7__INCLUDED_BmmD)
