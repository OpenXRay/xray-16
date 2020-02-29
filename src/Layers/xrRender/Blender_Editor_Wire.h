#ifndef BLENDER_EDITOR_WIRE_H
#define BLENDER_EDITOR_WIRE_H
#pragma once

class CBlender_Editor_Wire : public IBlender
{
    string64 oT_Factor;

public:
    virtual const char* getComment() { return "EDITOR: wire"; }
    virtual bool canBeLMAPped() { return FALSE; }
    virtual void Save(IWriter& fs);
    virtual void Load(IReader& fs, u16 version);

    virtual void Compile(CBlender_Compile& C);

    CBlender_Editor_Wire();
    virtual ~CBlender_Editor_Wire();
};

#endif // BLENDER_EDITOR_WIRE_H
