#pragma once

class CBlender_sunshafts : public IBlender
{
public:
    virtual		LPCSTR		getComment() { return "OGSE: sunshafts"; }
    virtual		BOOL		canBeDetailed() { return FALSE; }
    virtual		BOOL		canBeLMAPped() { return FALSE; }

    virtual		void		Compile(CBlender_Compile& C);

    CBlender_sunshafts();
    virtual ~CBlender_sunshafts();
};
