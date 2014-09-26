#ifndef BLENDER_BLUR_H
#define BLENDER_BLUR_H
#pragma once

class CBlender_Blur : public IBlender  
{
public:
	virtual		LPCSTR		getComment()	{ return "INTERNAL: blur";	}
	virtual		BOOL		canBeLMAPped()	{ return FALSE; }

	virtual		void		Save			(IWriter&  fs);
	virtual		void		Load			(IReader&	fs, u16 version);

	virtual		void		Compile			(CBlender_Compile& C);

	CBlender_Blur();
	virtual ~CBlender_Blur();
};

#endif //BLENDER_BLUR_H
