#pragma once
#ifndef ObjectSpaceRender_included
#define ObjectSpaceRender_included

#ifdef DEBUG
struct Fsphere;

class IObjectSpaceRender
{
public:
    virtual ~IObjectSpaceRender() { ; }
    virtual void Copy(IObjectSpaceRender& _in) = 0;

    virtual void dbgRender() = 0;
    virtual void dbgAddSphere(const Fsphere& sphere, u32 colour) = 0;
    virtual void dbgReserveSphere(size_t count) = 0;
    virtual void SetShader() = 0;
};
#endif // DEBUG

#endif //	ObjectSpaceRender_included
