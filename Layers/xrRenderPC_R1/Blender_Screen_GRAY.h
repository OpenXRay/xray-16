// Blender_Screen_GRAY.h: interface for the CBlender_Screen_GRAY class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BLENDER_SCREEN_GRAY_H__483E49EF_23EC_4810_9231_7EE4BD72CC3B__INCLUDED_)
#define AFX_BLENDER_SCREEN_GRAY_H__483E49EF_23EC_4810_9231_7EE4BD72CC3B__INCLUDED_
#pragma once

class CBlender_Screen_GRAY : public IBlender  
{
public:
	virtual		LPCSTR		getComment()	{ return "INTERNAL: gray-scale effect"; }
	virtual		BOOL		canBeLMAPped()	{ return FALSE; }
	
	virtual		void		Save			(IWriter&  fs);
	virtual		void		Load			(IReader&	fs, u16 version);
	
	virtual		void		Compile			(CBlender_Compile& C);
	
	CBlender_Screen_GRAY();
	virtual ~CBlender_Screen_GRAY();

};

#endif // !defined(AFX_BLENDER_SCREEN_GRAY_H__483E49EF_23EC_4810_9231_7EE4BD72CC3B__INCLUDED_)
