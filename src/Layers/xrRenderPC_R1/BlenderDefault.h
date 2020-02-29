// BlenderDefault.h: interface for the CBlenderDefault class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BLENDERDEFAULT_H__C12F64EE_43E7_4483_9AC3_29272E0401E7__INCLUDED_)
#define AFX_BLENDERDEFAULT_H__C12F64EE_43E7_4483_9AC3_29272E0401E7__INCLUDED_
#pragma once

class CBlender_default : public IBlender
{
public:
    virtual const char* getComment() { return "LEVEL: lmap*base (default)"; }
    virtual bool canBeDetailed() { return TRUE; }
    virtual bool canBeLMAPped() { return TRUE; }
    virtual void Save(IWriter& fs);
    virtual void Load(IReader& fs, u16 version);

    virtual void Compile(CBlender_Compile& C);

    CBlender_default();
    virtual ~CBlender_default();

private:
    xrP_TOKEN oTessellation;
};

#endif // !defined(AFX_BLENDERDEFAULT_H__C12F64EE_43E7_4483_9AC3_29272E0401E7__INCLUDED_)
