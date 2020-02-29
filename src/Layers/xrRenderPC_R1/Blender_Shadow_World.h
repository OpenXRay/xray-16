#ifndef BLENDER_SHADOW_WORLD_H
#define BLENDER_SHADOW_WORLD_H
#pragma once

class CBlender_ShWorld : public IBlender
{
public:
    virtual const char* getComment() { return "INTERNAL: shadow projecting"; }
    virtual bool canBeLMAPped() { return FALSE; }
    virtual void Save(IWriter& fs);
    virtual void Load(IReader& fs, u16 version);

    virtual void Compile(CBlender_Compile& C);

    CBlender_ShWorld();
    virtual ~CBlender_ShWorld();
};

#endif // BLENDER_SHADOW_WORLD_H
