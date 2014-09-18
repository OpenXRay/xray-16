#ifndef BLENDER_EDITOR_SELECTION_H
#define BLENDER_EDITOR_SELECTION_H
#pragma once

class CBlender_Editor_Selection : public IBlender  
{
	string64	oT_Factor;
public:
	virtual		LPCSTR		getComment()	{ return "EDITOR: selection"; }
	virtual		BOOL		canBeLMAPped()	{ return FALSE; }
	
	virtual		void		Save			(IWriter&  fs);
	virtual		void		Load			(IReader&	fs, u16 version);
	
	virtual		void		Compile			(CBlender_Compile& C);
	
	CBlender_Editor_Selection();
	virtual ~CBlender_Editor_Selection();

};

#endif //BLENDER_EDITOR_SELECTION_H
