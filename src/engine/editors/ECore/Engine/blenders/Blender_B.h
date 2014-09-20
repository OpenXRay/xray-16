// BlenderDefault.h: interface for the CBlenderDefault class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BLENDERDEFAULT_H__C12F64EE_43E7_4483_9AC3_29272E0401E7__INCLUDED_BmmD2)
#define AFX_BLENDERDEFAULT_H__C12F64EE_43E7_4483_9AC3_29272E0401E7__INCLUDED_BmmD2
#pragma once

class CBlender_B : public IBlender  
{
public:
	virtual		LPCSTR		getComment()	{ return "LEVEL: Implicit";	}
	virtual		BOOL		canBeDetailed()	{ return TRUE; }
	virtual		BOOL		canBeLMAPped()	{ return FALSE; }

	virtual		void		Save			(IWriter&  fs);
	virtual		void		Load			(IReader&	fs, u16 version);

	virtual		void		Compile			(CBlender_Compile& C);

	CBlender_B();
	virtual ~CBlender_B();
};

#endif // !defined(AFX_BLENDERDEFAULT_H__C12F64EE_43E7_4483_9AC3_29272E0401E7__INCLUDED_BmmD2)
