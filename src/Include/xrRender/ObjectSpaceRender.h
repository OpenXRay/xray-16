#pragma once
#ifndef ObjectSpaceRender_included
#define ObjectSpaceRender_included

#ifdef DEBUG

template <class T>
struct _sphere;
typedef _sphere<float> Fsphere;

class IObjectSpaceRender
{
public:
    virtual ~IObjectSpaceRender() { ; }
    virtual void Copy(IObjectSpaceRender& _in) = 0;

    virtual void dbgRender() = 0;
    virtual void dbgAddSphere(const Fsphere& sphere, u32 colour) = 0;
    virtual void SetShader() = 0;
};

#endif // DEBUG

#endif //	ObjectSpaceRender_included
